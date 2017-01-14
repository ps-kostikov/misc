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

QUESTS = u'🗺 Квесты'
CASTLE = u'🏰Замок'
ARENA = u'📯Арена'
DO_SEARCH = u'🔎Поиск соперника'
CANCEL_SEARCH = u'✖️Отменить поиск'

# ------ not checked
BLACK = u'🇬🇵'

BACK = u'⬅️Назад'
KOROVANY = u'🐫ГРАБИТЬ КОРОВАНЫ'

TAVERN = u'🍺Таверна'

HIT_HEAD = u'🗡в голову'
HIT_TORSO = u'🗡по корпусу'
HIT_LEGS = u'🗡по ногам'
DEF_HEAD = u'🛡головы'
DEF_TORSO = u'🛡корпуса'
DEF_LEGS = u'🛡ног'

HITS = [
    HIT_HEAD,
    HIT_TORSO,
    HIT_LEGS,
]
DEFS = [
    DEF_HEAD,
    DEF_TORSO,
    DEF_LEGS,
]
HITS_DEFS = HITS + DEFS
# -------

DEFAULT_TARGET = WHITE

@coroutine
def enqueue_msgs():
    try:
        while True:
            msg = (yield)

            if not hasattr(msg, 'event'):
                continue
            if not hasattr(msg, 'own'):
                continue
            if not hasattr(msg, 'text'):
                continue
            if not hasattr(msg, 'sender'):
                continue
            if not hasattr(msg.sender, 'username'):
                continue


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
    except:
        logger.exception("receiver dies")
    # except GeneratorExit:
    #     pass
    # except KeyboardInterrupt:
    #     pass
    else:
        logger.info("receiver quit")

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
            exit_options = [
                u'Вы вернулись из леса',
                u'Вы заработали',
                u'Ты заработал',
            ]
            for t in exit_options:
                if t in msg.text:
                    logger.info("wait_forest_done complete")
                    return True
        time.sleep(delay)
    logger.info('time is out; quit forest')
    return False

def wait_arena_search_done():
    logger.info('wait_arena_search_done')
    sec_to_wait = 10 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        while not chat_wars_msg_queue.empty():
            msg = chat_wars_msg_queue.get()
            if u'У тебя 30 секунд' in msg.text:
                logger.info("wait_arena_search_done complete")
                return True
        time.sleep(delay)

    return False

def wait_any():
    logger.info('wait any')
    sec_to_wait = 15 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        while not chat_wars_msg_queue.empty():
            msg = chat_wars_msg_queue.get()
            logger.info("wait any complete")
            return True
        time.sleep(delay)
    logger.info('time is out; quit forest')
    return False




def do_forest(sender):
    logger.info("send quests")
    sender.send_msg(CHAT_WARS_PEER, QUESTS)
    wait_any()
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

def do_battle_step(sender, options):
    logger.info("send hit def")
    sender.send_msg(CHAT_WARS_PEER, random.choice(options))
    sec_to_wait = 3 * 60
    delay = 1
    for i in range(sec_to_wait / delay):
        while not chat_wars_msg_queue.empty():
            msg = chat_wars_msg_queue.get()
            print msg.text
            if u'У тебя 30 секунд' in msg.text:
                return HITS_DEFS
            elif u'Выбери место удара' in msg.text:
                return HITS
            elif u'определиться с защитой' in msg.text:
                return DEFS
            elif u'Таблица победителей обновлена' in msg.text:
                return []
            elif u'посмотрим что из этого выйдет' in msg.text:
                continue
            else:
                logger.info("unexpected battle answer")
                return HITS_DEFS
        time.sleep(delay)
    logger.info('time is out; quit battle step')
    return []

def do_arena(sender):
    logger.info("send castle")
    sender.send_msg(CHAT_WARS_PEER, CASTLE)
    wait_any()
    logger.info("send arena")
    sender.send_msg(CHAT_WARS_PEER, ARENA)
    wait_any()
    logger.info("send search")
    sender.send_msg(CHAT_WARS_PEER, DO_SEARCH)
    res = wait_arena_search_done()
    if not res:
        logger.info("send cancel search")
        sender.send_msg(CHAT_WARS_PEER, CANCEL_SEARCH)
        return

# u'посмотрим что из этого выйдет'
# u'Таблица победителей обновлена'
# u'Соперник найден'
# u'Выбери место удара'
# u'У тебя 30 секунд'
# u'определиться с защитой'

    options = HITS_DEFS
    while True:
        print options
        next_options = do_battle_step(sender, options)
        if not next_options:
            break
        options = next_options



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

    time.sleep(1)
    sender = Sender(host="localhost", port=4458)

    # for i in range(6):
    #     do_forest(sender)
    # return

    do_arena(sender)

    # for color in (BLUE, RED, WHITE, YELLOW):
    #     do_attack(sender, color)

    # do_defence(sender)

    # sender.send_msg(CHAT_WARS_PEER, DEFENCE_COMMAND)
    # time.sleep(1)

    # sender.send_msg(CHAT_WARS_PEER, BACK)
    # time.sleep(1)
    # return

    try:
        while True:
            while not chat_wars_msg_queue.empty():
                msg = chat_wars_msg_queue.get()
                if GO in msg.text:
                    do_go(sender)
                    # do_defence(sender)
                    do_attack(sender, DEFAULT_TARGET)

            # while not command_chat_msg_queue.empty():
            #     msg = command_chat_msg_queue.get()
            #     if BLUE in msg.text:
            #         do_attack(sender, BLUE)
            #     elif RED in msg.text:
            #         do_attack(sender, RED)
            #     elif YELLOW in msg.text:
            #         do_attack(sender, YELLOW)
            #     elif WHITE in msg.text:
            #         do_attack(sender, WHITE)
            #     elif DEFENCE_COMMAND in msg.text:
            #         do_defence(sender)

            time.sleep(1)
            # time.sleep(random.randint(20, 40))
    except:
        logger.exception('bum')

if __name__ == '__main__':
    main()