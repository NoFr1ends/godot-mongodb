extends MainLoop

var db: MongoDB = null

func _init():
    db = MongoDB.new()
    db.connect_database("mongodb://127.0.0.1")
    print("connected")

    list_accounts()

func _idle(_delta):
    db.poll()

    return false

func list_accounts():
    var accounts = db.get_database("test").get_collection("test")
    var result = accounts.find({})
    yield(result, "completed")
    print(result.data)