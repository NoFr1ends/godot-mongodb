extends SceneTree

var db: MongoDB = null

func _init():
    db = MongoDB.new()
    db.connect_database("mongodb://test:test123@127.0.0.1?authSource=admin")
    print("connected")

    #db.auth("test", "test123", "admin");

    #test_cursor()
    #list_accounts()
    #test_update()
    #test_delete()
    #test_insert()
    #test_find_and_update()

func _idle(_delta):
    db.poll()

    return false

func test_cursor():
    var start = OS.get_ticks_msec()

    var dummy = db.get_database("test").get_collection("dummy")
    var result = dummy.find({})
    var length = 0
    var pages = 0

    while result.next():
        yield(result, "completed")

        # do whatever you want with the page of data
        length += result.data.size()
        pages += 1

    var end = OS.get_ticks_msec()

    print("Found ", length, " objects in ", (end-start), "ms using ", pages, " pages")

func list_accounts():
    var accounts = db.get_database("test").get_collection("test")
    var result = accounts.find({})
    result.next()
    yield(result, "completed")
    print("find: ", result.data)

    result = accounts.find_one({"test": 4711})
    yield(result, "completed")
    print("find_one: ", result.data)

    result = accounts.find_one({"test": 1337})
    yield(result, "completed")
    print("find_one: ", result.data)

func test_update():
    var accounts = db.get_database("test").get_collection("test")
    yield(accounts.update_one({"test": 4712}, {"$set": {"gd": "script"}}), "completed")
    print("update one done")
    
    yield(accounts.update_many({}, {"$set": {"time": OS.get_unix_time()}}), "completed")
    print("update many done")

func test_delete():
    var accounts = db.get_database("test").get_collection("test")
    yield(accounts.delete_one({"test": 4711}), "completed")
    print("delete one done")

    yield(accounts.delete_many({}), "completed")
    print("delete many done")

func test_insert():
    var accounts = db.get_database("test").get_collection("test")
    yield(accounts.insert_one({"test": 4711, "gd": "insert"}), "completed")
    print("insert one done")

func test_find_and_update():
    var accounts = db.get_database("test").get_collection("test")
    var result = yield(accounts.find_one_and_update({"test": 4711}, {"$set": {"gd": "and_update"}}, true), "completed")
    if result:
        print("done updating, new document: ", result.data)
    else:
        print("failed to update document!")