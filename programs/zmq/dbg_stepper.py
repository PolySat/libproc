import os
import sys
import tornado.ioloop
import sockjs.tornado
import time

#import zmq and install zmq's event loop as part of tornado
import zmq
from zmq.eventloop import ioloop
from zmq.eventloop.zmqstream import ZMQStream
ioloop.install()

import json

root = os.path.dirname(__file__)

def setup_zmq_sub():
    ctx = zmq.Context()
    s = ctx.socket(zmq.PAIR)
    s.connect('tcp://localhost:52003')
#    s.send_string('{"command":"step"}')
    stream = ZMQStream(s)

    def update_debug_state(msg):
        data = json.loads(msg[0].decode('utf-8'))
        print(json.dumps(data, indent=4))
        print(data['dbg_state'])
#if data['dbg_state'] == 'stopped':
#s.send_string('{"command":"next"}')

    stream.on_recv(update_debug_state)

if __name__ == '__main__':
    setup_zmq_sub()    
    tornado.ioloop.IOLoop.instance().start()
