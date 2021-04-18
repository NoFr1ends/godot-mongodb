#include "scram_auth.h"

#include "core/io/marshalls.h"
#include "core/crypto/crypto_core.h"

#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>

void ScramAuth::prepare(String username, String password, String database) {
    // Temporarly store connection details
    m_username = username;
    m_password = password;
    m_database = database;

    // Generate random nonce
    // FIXME: not cryptographical secure! only "better" pseudo random numbers
    RandomPCG random;
    random.randomize();

    m_nonce = PoolByteArray();
    m_nonce.resize(24);

    auto write = m_nonce.write();

    for(int i = 0; i < 24; i += 4) {
        auto rand = random.rand();
        encode_uint32(rand, &write[i]);
    }
}

void ScramAuth::auth(Ref<MongoDB> mongo) {
    m_mongo = mongo;
    send_init(mongo);
}

String ScramAuth::first_message() {
    return "n=" + m_username + ",r=" + CryptoCore::b64_encode_str(m_nonce.read().ptr(), m_nonce.size());
}

void ScramAuth::send_init(Ref<MongoDB> mongo) {
    String payload = "n,," + first_message();
    CharString utf8 = payload.utf8();
    PoolByteArray payload_buffer;
    size_t len = utf8.length();
    payload_buffer.resize(len);
    PoolByteArray::Write w = payload_buffer.write();
    copymem(w.ptr(), utf8.ptr(), len);
    w.release();

    Dictionary sasl_start;
    sasl_start["saslStart"] = 1;
    sasl_start["mechanism"] = "SCRAM-SHA-256";
    sasl_start["payload"] = payload_buffer;
    sasl_start["autoAuthorize"] = 1;
    sasl_start["$db"] = m_database;
    Dictionary options;
    options["skipEmptyExchange"] = true;
    sasl_start["options"] = options;
    mongo->execute_msg(this, sasl_start, 0);
}

String ScramAuth::password_digest() {
    String str = m_username + ":mongo:" + m_password;
    return str.md5_text();
}

PoolByteArray base64_raw(String base64) {
    int strlen = base64.length();
	CharString cstr = base64.ascii();

	size_t arr_len = 0;
	PoolVector<uint8_t> buf;
	{
		buf.resize(strlen / 4 * 3 + 1);
		PoolVector<uint8_t>::Write w = buf.write();

		ERR_FAIL_COND_V(CryptoCore::b64_decode(&w[0], buf.size(), &arr_len, (unsigned char *)cstr.get_data(), strlen) != OK, PoolVector<uint8_t>());
	}
	buf.resize(arr_len);

	return buf;
}

PoolByteArray hmac(mbedtls_md_context_t *ctx, PoolByteArray &key, String data) {
    PoolByteArray output;
    output.resize(32);

    CharString cstr = data.utf8();
    auto key_reader = key.read();
    auto output_writer = output.write();

    mbedtls_md_hmac_starts(ctx, key_reader.ptr(), key.size());
    mbedtls_md_hmac_update(ctx, reinterpret_cast<const unsigned char*>(cstr.ptr()), data.size() - 1);
    mbedtls_md_hmac_finish(ctx, output_writer.ptr());

    return output;
}

PoolByteArray hash(mbedtls_md_context_t *ctx, PoolByteArray data) {
    PoolByteArray output;
    output.resize(32);

    mbedtls_md_starts(ctx);
    mbedtls_md_update(ctx, data.read().ptr(), data.size());
    mbedtls_md_finish(ctx, output.write().ptr());

    return output;
}

String xor_buffer(PoolByteArray a, PoolByteArray b) {
    ERR_FAIL_COND_V_MSG(a.size() != b.size(), "", "Both buffer must have the same size!");

    PoolByteArray c;
    c.resize(a.size());
    for(auto i = 0; i < c.size(); i++) {
        c.set(i, a[i] ^ b[i]);
    }

    return CryptoCore::b64_encode_str(c.read().ptr(), c.size());
}

void ScramAuth::process_msg(Dictionary &reply) {
    if((int)reply["ok"] != 1) {
        // auth failed
        return;
    }

    // Check if we're done
    if((bool)reply["done"]) {
        // TODO: check server signature!

        // auth done
        print_line("authentication done");
        return;
    }

    int conversation_id = reply["conversationId"];
    PoolByteArray payload_buffer = reply["payload"];
    String payload;
    PoolByteArray::Read r = payload_buffer.read();
    payload.parse_utf8((const char *)r.ptr(), payload_buffer.size());
    r.release();

    // Parse reply
    auto arguments_list = payload.split(",");
    Dictionary arguments;
    for(int i = 0; i < arguments_list.size(); i++) {
        auto a = arguments_list[i].split("=", true, 1);
        ERR_CONTINUE_MSG(a.size() != 2, "Failed to parse SCRAM payload: " + arguments_list[i]);
        arguments[a[0]] = a[1];
    }

    int iterations = ((String)arguments["i"]).to_int();
    if(iterations && iterations < 4096) {
        ERR_FAIL_MSG("Server returned invalid iteration count " + itos(iterations));
    }

    String salt = arguments["s"];
    String rnonce = arguments["r"];
    PoolByteArray salt_buffer = base64_raw(salt);
    auto salt_reader = salt_buffer.read();

    print_verbose("SCRAM reply: ");
    print_verbose((Variant)arguments);

    String pwd = m_password; //password_digest();
    CharString pwd_ascii = pwd.ascii();

    // Start proof
    String without_proof = "c=biws,r=" + rnonce;

    // TODO: clean this mass up
    // generating salted password
    mbedtls_md_context_t ctx;
    PoolByteArray salted_data;
    salted_data.resize(32);
    auto salted_data_writer = salted_data.write();
    mbedtls_md_init(&ctx);
    auto info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, info, 1);
    auto status = mbedtls_pkcs5_pbkdf2_hmac(&ctx, reinterpret_cast<const unsigned char*>(pwd_ascii.ptr()), pwd_ascii.size(), salt_reader.ptr(), salt_buffer.size(), iterations, 32, salted_data_writer.ptr());
    ERR_FAIL_COND_MSG(status != 0, "Failed to PBKDF2: " + itos(status));

    print_verbose(pwd);
    print_verbose(salt);
    print_verbose(CryptoCore::b64_encode_str(salted_data.read().ptr(), salted_data.size()));

    // generate client and server key
    auto client_key = hmac(&ctx, salted_data, "Client Key");
    auto server_key = hmac(&ctx, salted_data, "Server Key");
    auto stored_key = hash(&ctx, client_key);
    print_verbose(CryptoCore::b64_encode_str(client_key.read().ptr(), client_key.size()));
    print_verbose(CryptoCore::b64_encode_str(server_key.read().ptr(), server_key.size()));
    print_verbose(CryptoCore::b64_encode_str(stored_key.read().ptr(), stored_key.size()));

    String auth_message = first_message() + "," + payload + "," + without_proof;
    auto client_signature = hmac(&ctx, stored_key, auth_message);
    print_verbose(CryptoCore::b64_encode_str(client_signature.read().ptr(), client_signature.size()));

    auto client_proof = "p=" + xor_buffer(client_key, client_signature);
    print_verbose(client_proof);
    auto final = without_proof + "," + client_proof;

    m_server_signature = hmac(&ctx, server_key, auth_message); // TODO: store that to compare on reply
    print_verbose(CryptoCore::b64_encode_str(m_server_signature.read().ptr(), m_server_signature.size()));

    CharString utf8 = final.utf8();
    PoolByteArray final_buffer;
    size_t len = utf8.length();
    final_buffer.resize(len);
    PoolByteArray::Write w = final_buffer.write();
    copymem(w.ptr(), utf8.ptr(), len);
    w.release();

    Dictionary sasl_continue;
    sasl_continue["saslContinue"] = 1;
    sasl_continue["conversationId"] = conversation_id;
    sasl_continue["payload"] = final_buffer;
    sasl_continue["$db"] = m_database;

    m_mongo->execute_msg(this, sasl_continue, 0);
}