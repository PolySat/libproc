import os
import sys
import tornado.ioloop
import tornado.web
import sockjs.tornado

#import zmq and install zmq's event loop as part of tornado
import zmq
from zmq.eventloop import ioloop
from zmq.eventloop.zmqstream import ZMQStream
ioloop.install()

import json

port = 3000
root = os.path.dirname(__file__)

class DebugDataHandler(tornado.web.RequestHandler):
    def get(self):
        self.write(json.dumps(self.application.last_debug_data))

class DebugDataBroadcast(sockjs.tornado.SockJSConnection):

    def on_open(self, info):
        clients.add(self)
        self.send(last_debug_data)

    def on_close(self):
        clients.remove(self)

def setup_zmq_sub():
    ctx = zmq.Context()
    s = ctx.socket(zmq.PAIR)
    s.connect('tcp://localhost:52003')
    s.send_string('{"command":"periodic_dump","ms":1000}')
#s.send_string('{"command":"run"}')
    stream = ZMQStream(s)

    def update_debug_state(msg):
        global last_debug_data

        data = json.loads(msg[0].decode('utf-8'))

        if (len(sys.argv) > 1 and sys.argv[1] == '-v'):
            print(json.dumps(data, indent=4))

        last_debug_data = data
        debug_data_stream.broadcast(clients, data)
        if data['dbg_state'] == 'stopped':
            print('Stopped')
            s.send_string('{"command":"run","ms":15000}')

    stream.on_recv(update_debug_state)

clients = set()
last_debug_data = {}
debug_data_stream = sockjs.tornado.SockJSRouter(DebugDataBroadcast, '/debug-socket')

application = tornado.web.Application(
    debug_data_stream.urls + [
    (r"/debug", DebugDataHandler),
    (r"/(.*)", tornado.web.StaticFileHandler, {"path": root, "default_filename": "index.html"}),
])

if __name__ == '__main__':
    print('Starting debugger server on http://localhost:' + str(port))
    setup_zmq_sub()    
    application.listen(port)
    tornado.ioloop.IOLoop.instance().start()
