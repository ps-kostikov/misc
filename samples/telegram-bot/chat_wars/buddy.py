#!/usr/bin/env python
# -*- coding: utf-8 -*-
#

from __future__ import unicode_literals
from pytg.receiver import Receiver  # get messages
from pytg.sender import Sender  # send messages, and other querys.
from pytg.utils import coroutine

from Queue import Queue
import threading
import time
import sys
import random
import logging

logger = logging.getLogger("buddy")

chat_wars_msg_queue = Queue()
command_chat_msg_queue = Queue()

CHAT_WARS_PEER = 'Chat_Wars'

# checked
FOREST = u"🌲Лес"
GO = u'/go'
DEFENCE = u'🛡 Защита'
DEFENCE_COMMAND = u'🛡'
ATTACK = u'⚔ Атака'
BLUE = u'🇪🇺'
RED = u'🇮🇲'
WHITE = u'🇨🇾'
YELLOW = u'🇻🇦'
HERO = u'🏅Герой'

# ------ not checked
BLACK = u'🇬🇵'
# -------

DEFAULT_TARGET = RED

@coroutine
def enqueue_msgs():
    try:
        while True:
            msg = (yield)

            # print '>>', msg
            if msg.event != "message":
                continue
            if msg.own:
                continue
            if msg.text is None:
                continue

            if msg.sender.username == 'ChatWarsBot':
                chat_wars_msg_queue.put(msg)
            if msg.sender.username == 'blackcastlebot':
                command_chat_msg_queue.put(msg)

    except GeneratorExit:
        pass
    except KeyboardInterrupt:
        pass
    else:
        pass

def run_receiver():
    receiver = Receiver(host="localhost", port=4458)
    receiver.start()
    logger.info('receiver.message...')
    receiver.message(enqueue_msgs())
    receiver.stop()


def wait_forest_done():
    logger.info('wait_forest_done')
    sec_to_wait = 15 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        while not chat_wars_msg_queue.empty():
            msg = chat_wars_msg_queue.get()
            # print msg
            if u'Вы вернулись из леса' in msg.text or u'Вы заработали' in msg.text:
                logger.info("wait_forest_done complete")
                return
        time.sleep(delay)
    logger.info('time is out; quit forest')

def wait_any():
    logger.info('wait any')
    sec_to_wait = 15 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        while not chat_wars_msg_queue.empty():
            msg = chat_wars_msg_queue.get()
            logger.info("wait any complete")
            return
        time.sleep(delay)
    logger.info('time is out; quit forest')


def do_forest(sender):
    logger.info("send forest")
    sender.send_msg(CHAT_WARS_PEER, FOREST)
    wait_forest_done()

def do_attack(sender, color):
    logger.info("send attack")
    sender.send_msg(CHAT_WARS_PEER, ATTACK)
    wait_any()
    logger.info("send flag")
    sender.send_msg(CHAT_WARS_PEER, color)
    wait_any()

def do_defence(sender):
    logger.info("send defence")
    sender.send_msg(CHAT_WARS_PEER, DEFENCE)
    wait_any()

def do_go(sender):
    logger.info("send go")
    sender.send_msg(CHAT_WARS_PEER, GO)
    wait_any()
    wait_any()


def setup_logger():
    hdlr = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter('%(asctime)s %(message)s')
    hdlr.setFormatter(formatter)
    logger.addHandler(hdlr)
    logger.setLevel(logging.DEBUG)


def main():
    setup_logger()

    thread = threading.Thread(target=run_receiver)
    thread.daemon = True
    thread.start()

    sender = Sender(host="localhost", port=4458)

    # for i in range(2):
    #     do_forest(sender)

    # for color in (BLUE, RED, WHITE, YELLOW):
    #     do_attack(sender, color)

    # do_defence(sender)

    # sender.send_msg(CHAT_WARS_PEER, DEFENCE_COMMAND)
    # time.sleep(1)

    try:
        while True:
            while not chat_wars_msg_queue.empty():
                msg = chat_wars_msg_queue.get()
                if GO in msg.text:
                    do_go(sender)
                    do_attack(sender, DEFAULT_TARGET)

            while not command_chat_msg_queue.empty():
                msg = command_chat_msg_queue.get()
                if BLUE in msg.text:
                    do_attack(sender, BLUE)
                elif RED in msg.text:
                    do_attack(sender, RED)
                elif YELLOW in msg.text:
                    do_attack(sender, YELLOW)
                elif WHITE in msg.text:
                    do_attack(sender, WHITE)
                elif DEFENCE_COMMAND in msg.text:
                    do_defence(sender)

            # time.sleep(30)
            time.sleep(random.randint(20, 40))
    except:
        logger.exception('bum')

if __name__ == '__main__':
    main()