extends MainLoop

var db: MongoDB = null

func _init():
    db = MongoDB.new()
    db.connect_database("mongodb://127.0.0.1")
    print("connected")

    test_cursor()
    list_accounts()

func _idle(_delta):
    db.poll()

    return false

func test_cursor():
    var dummy = db.get_database("test").get_collection("dummy")
    var result = dummy.find({})
    yield(result, "completed")
    print("find: ", result.data.size())

func list_accounts():
    var accounts = db.get_database("test").get_collection("test")
    var result = accounts.find({})
    yield(result, "completed")
    print("find: ", result.data)

    result = accounts.find_one({"test": 4711})
    yield(result, "completed")
    print("find_one: ", result.data)

    result = accounts.find_one({"test": 1337})
    yield(result, "completed")
    print("find_one: ", result.data)