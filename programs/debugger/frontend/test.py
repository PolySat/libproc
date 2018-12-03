import zmq
import json

context = zmq.Context()
socket = context.socket(zmq.SUB)

socket.connect("tcp://localhost:5565")
topicfilter = ""
socket.setsockopt_string(zmq.SUBSCRIBE, topicfilter)

while True:
    print("Receiving")
    msg = socket.recv_json()

    print(json.dumps(msg, indent=4))

