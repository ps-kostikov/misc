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
import datetime
import collections

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

DEFAULT_TARGET = BLUE

STATE_REST = 'rest'
STATE_ATTACK_RED = 'attack_red'
STATE_ATTACK_BLUE = 'attack_blue'
STATE_ATTACK_WHITE = 'attack_white'
STATE_ATTACK_YELLOW = 'attack_yellow'
STATE_ATTACK_BLACK = 'attack_black'
STATE_DEFENCE = 'defence'
STATE_FOREST = 'forest'
STATE_KOROVANY = 'korovany'
STATE_ARENA = 'arena'

HeroStatus = collections.namedtuple("HeroStatus", "gold stamina stamina_max state")


# TODO fill all possible msg types
CW_MSG_TYPE_HERO = 'hero'
CW_MSG_TYPE_UNKNOWN = 'unknown'


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
            if not hasattr(msg, 'date'):
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

def get_last_msg(queue, queue_id):
    msgs = []
    while not queue.empty():
        msgs.append(queue.get())
    if not msgs:
        return None

    def text_to_log(text):
        parts = text.split('\n')
        if not parts:
            return ''
        if len(parts) == 1:
            return parts[0]
        return parts[0] + u' ...'
    for msg in msgs:
        logger.info(u'msg read from {0!r} at {1}: {2}'.format(
            queue_id,
            datetime.datetime.fromtimestamp(msg.date),
            text_to_log(msg.text)))
    return msgs[-1]

def get_last_chat_wars_msg():
    return get_last_msg(chat_wars_msg_queue, 'chat wars')

def get_last_command_chat_msg():
    return get_last_msg(command_chat_msg_queue, 'command chat')

def send_msg(sender, text):
    logger.info(u"send msg to 'chat wars' at {0}: {1}".format(
        datetime.datetime.now(),
        text))
    sender.send_msg(CHAT_WARS_PEER, text)


def time_to_battle():
    now = datetime.datetime.now()
    battle_time = (
        datetime.datetime.combine(
            datetime.date.today(),
            datetime.time.min
        )
        + datetime.timedelta(hours=1)
    )
    battle_gap = datetime.timedelta(hours=3)
    while battle_time < now:
        battle_time += battle_gap
    return battle_time - now


def wait_forest_done():
    logger.info('wait_forest_done')
    sec_to_wait = 15 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
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
    sec_to_wait = 20 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
            if u'У тебя 30 секунд' in msg.text:
                logger.info("wait_arena_search_done complete")
                return True
            if u'Сражаться можно не чаще' in msg.text:
                logger.info("wait_arena_search_done not successfull")
                return False
        time.sleep(delay)

    return False

def wait_any():
    logger.info('wait any')
    sec_to_wait = 15 * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
            logger.info("wait any complete")
            return True
        time.sleep(delay)
    logger.info('time is out; quit forest')
    return False




def do_forest(sender):
    send_msg(sender, QUESTS)
    wait_any()
    send_msg(sender, FOREST)
    wait_forest_done()

def do_attack(sender, color):
    send_msg(sender, ATTACK)
    wait_any()
    send_msg(sender, color)
    wait_any()

def do_defence(sender):
    send_msg(sender, DEFENCE)
    wait_any()

def do_go(sender):
    send_msg(sender, GO)
    wait_any()
    wait_any()

def do_battle_step(sender, options):
    send_msg(sender, random.choice(options))
    sec_to_wait = 3 * 60
    delay = 3
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
            # print msg.text
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
    send_msg(sender, CASTLE)
    wait_any()
    send_msg(sender, ARENA)
    wait_any()
    send_msg(sender, DO_SEARCH)
    res = wait_arena_search_done()
    if not res:
        send_msg(sender, CANCEL_SEARCH)
        return

# u'Технические работы'
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

def get_hero_status(sender):
    # TODO
    pass


def setup_logger():
    hdlr = logging.StreamHandler(sys.stdout)
    formatter = logging.Formatter('%(asctime)s %(message)s')
    hdlr.setFormatter(formatter)
    logger.addHandler(hdlr)
    logger.setLevel(logging.DEBUG)


def main():
    setup_logger()

    # print time_to_battle()
    # return

    thread = threading.Thread(target=run_receiver)
    thread.daemon = True
    thread.start()

    time.sleep(1)
    sender = Sender(host="localhost", port=4458)

    # while True:
    #     get_last_chat_wars_msg()
    #     time.sleep(10)
    # return


    # for i in range(8):
    #     do_forest(sender)
    # return

    # wait_any()
    # return
    print time_to_battle()
    do_arena(sender)
    return 

    # for color in (BLUE, RED, WHITE, YELLOW):
    #     do_attack(sender, color)

    # do_defence(sender)

    # send_msg(sender, DEFENCE_COMMAND)
    # time.sleep(1)

    # send_msg(sender, BACK)
    # time.sleep(1)
    # return

    try:
        while True:
            msg = get_last_chat_wars_msg()
            if msg is not None:
                if GO in msg.text:
                    do_go(sender)
                    # do_defence(sender)
                    do_attack(sender, DEFAULT_TARGET)

            msg = get_last_command_chat_msg()
            if msg is not None:
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

            # time.sleep(1)
            time.sleep(random.randint(20, 40))
    except:
        logger.exception('bum')

if __name__ == '__main__':
    main()