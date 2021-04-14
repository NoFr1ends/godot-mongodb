extends MainLoop

var db: MongoDB = null

func _init():
    db = MongoDB.new()
    db.connect_database("mongodb://127.0.0.1")
    print("connected")

    #test_cursor()
    #list_accounts()
    test_update()

func _idle(_delta):
    db.poll()

    return false

func test_cursor():
    var start = OS.get_ticks_msec()

    var dummy = db.get_database("test").get_collection("dummy")
    var result = dummy.find({})
    var length = 0

    while result.next():
        yield(result, "completed")

        # do whatever you want with the page of data
        length += result.data.size()

    var end = OS.get_ticks_msec()

    print("Found ", length, " objects in ", (end-start), "ms")

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

func test_update():
    var accounts = db.get_database("test").get_collection("test")
    accounts.update_one({"test": 4712}, {"$set": {"gd": "script"}})
    
    accounts.update_many({}, {"$set": {"time": OS.get_unix_time()}})