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
import re
import os
import pytz

logger = logging.getLogger("buddy")
TZ = pytz.timezone('Europe/Moscow')

chat_wars_msg_queue = Queue()
command_chat_msg_queue = Queue()
self_msg_queue = Queue()

CHAT_WARS_PEER = 'Chat_Wars'
SELF_PEER = 'Pavel_Kostikov'

sender = None


# checked
FOREST_QUEST = u"🌲Лес"
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

TAVERN = u'🍺Таверна'
TAKE_ELL = u'🍺Взять кружку эля'


# ------ not checked
BLACK = u'🇬🇵'

BACK = u'⬅️Назад'
KOROVANY = u'🐫ГРАБИТЬ КОРОВАНЫ'
CAVE_QUEST = u'🕸Пещера'

MOUNTAIN = u'⛰'
MOUNTAIN_FORT = u'⛰Горный форт'
FOREST_FORT = u'🌲Лесной форт'
FOREST = u"🌲"

ATTACK_SIGN = u'⚔'

#---------------------------

STATE_REST = 'rest'
STATE_ATTACK_RED = 'attack_red'
STATE_ATTACK_BLUE = 'attack_blue'
STATE_ATTACK_WHITE = 'attack_white'
STATE_ATTACK_YELLOW = 'attack_yellow'
STATE_ATTACK_BLACK = 'attack_black'
STATE_ATTACK_FOREST_FORT = 'attack_forest_fort'
STATE_ATTACK_MOUNTAIN_FORT = 'attack_mountain_fort'
STATE_DEFENCE = 'defence'
STATE_DEFENCE_FOREST_FORT = 'defence_forest_fort'
STATE_DEFENCE_MOUNTAIN_FORT = 'defence_mountain_fort'
STATE_FOREST = 'forest'
STATE_TAVERN = 'tavern'
STATE_KOROVANY = 'korovany'
STATE_ARENA = 'arena'
STATE_UNKNOWN = 'unknown'

TEAM_RED = 'red'
TEAM_BLUE = 'blue'
TEAM_WHITE = 'white'
TEAM_YELLOW = 'yellow'
TEAM_BLACK = 'black'
TEAM_UNKNOWN = 'unknown'

HeroStatus = collections.namedtuple("HeroStatus", "team gold stamina stamina_max state")


# TODO fill all possible msg types
CW_MSG_TYPE_HERO = 'hero'
CW_MSG_TYPE_UNKNOWN = 'unknown'

#-------------------------------------

# u'Технические работы'
# u'посмотрим что из этого выйдет'
# u'Таблица победителей обновлена'
# u'Соперник найден'
# u'Выбери место удара'
# u'У тебя 30 секунд'
# u'определиться с защитой'


BUDDY_STATE_WORKING = 'working'
BUDDY_STATE_PAUSED = 'paused'

buddy_state = None

class PauseException(Exception):
    pass

#-----------------------------------


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
            # if msg.own:
            #     continue
            if msg.text is None:
                continue

            if msg.sender.username == 'ChatWarsBot':
                chat_wars_msg_queue.put(msg)
            if msg.sender.username == 'blackcastlebot':
                command_chat_msg_queue.put(msg)
            if msg.sender.username == 'ps_kostikov' and msg.text.lower().startswith(u'com'):
                self_msg_queue.put(msg)
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


def send_buddy_status():
    send_msg("I'm " + buddy_state, SELF_PEER)

def do_control_stuff():
    global buddy_state

    if self_msg_queue.empty():
        return
    while not self_msg_queue.empty():
        last_msg = self_msg_queue.get()
    text = last_msg.text
    if text.endswith('s'):
        send_buddy_status()
    elif text.endswith('p'):
        if buddy_state == BUDDY_STATE_WORKING:
            raise PauseException()
        send_buddy_status()
    elif text.endswith('c'):
        if buddy_state == BUDDY_STATE_PAUSED:
            buddy_state = BUDDY_STATE_WORKING
        send_buddy_status()
    else:
        send_msg("I don't understand", SELF_PEER)

def get_last_msg(queue, queue_id):
    do_control_stuff()

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
            datetime.datetime.fromtimestamp(msg.date, TZ),
            text_to_log(msg.text)))
    return msgs[-1]

def get_last_chat_wars_msg():
    return get_last_msg(chat_wars_msg_queue, 'chat wars')

def get_last_command_chat_msg():
    return get_last_msg(command_chat_msg_queue, 'command chat')

def get_last_self_msg():
    return get_last_msg(self_msg_queue, 'self')

def send_msg(text, peer=CHAT_WARS_PEER):
    logger.info(u"send msg to {0!r} at {1}: {2}".format(
        peer,
        datetime.datetime.now(TZ),
        text))
    sender.send_msg(peer, text)


def time_to_battle():
    now = datetime.datetime.now(TZ)
    battle_time = now - datetime.timedelta(
        hours=now.time().hour,
        minutes=now.time().minute,
        seconds=now.time().second,
        microseconds=now.time().microsecond)
    battle_gap = datetime.timedelta(hours=4)
    while battle_time < now:
        battle_time += battle_gap
    return battle_time - now


RECIPES_FILE = 'recipes.txt'
def append_recipe_candidate(text):
    delim = '\n\n\n'
    encoding = 'utf-8'
    if os.path.exists(RECIPES_FILE):
        with open(RECIPES_FILE) as inf:
            data = inf.read().decode(encoding)
            recipes = set(data.split(delim))
    else:
        recipes = set()

    recipes.add(text)
    with open(RECIPES_FILE, 'w') as outf:
        data = delim.join(recipes)
        outf.write(data.encode(encoding))


def parse_hero_status(text):

    team = None
    gold = None
    stamina = None
    stamina_max = None
    state = None

    def split_stat(s):
        parts = map(int, s.split('+'))
        if len(parts) == 1:
            return parts[0], 0
        return parts[0], parts[1]

    for line in text.split('\n'):
        match = re.match(u'..(.*),\s+Воин\s+(\W+)\s+замка.*', line)
        if match is not None:
            _, castle_word = match.groups()
            word_to_team = {
                u'Черного': TEAM_BLACK,
                u'Белого': TEAM_WHITE,
                u'Синего': TEAM_BLUE,
                u'Желтого': TEAM_YELLOW,
                u'Красного': TEAM_RED,
            }
            if castle_word in word_to_team.keys():
                team = word_to_team[castle_word]
            continue

        match = re.match(u'.*Золото:\s+(\d+).*', line)
        if match is not None:
            gold = int(match.groups()[0])
            continue

        match = re.match(u'.*Выносливость:\s+(\d+)\s+из\s+([\d\+]+).*', line)
        if match is not None:
            stamina = int(match.groups()[0])
            stamina_max = sum(split_stat(match.groups()[1]))
            continue

        match = re.match(u'.*Атака на\s+..(\W+)\s+замок.*', line)
        if match is not None:
            castle_word = match.groups()[0]
            word_to_state = {
                u'Черный': STATE_ATTACK_BLACK,
                u'Белый': STATE_ATTACK_WHITE,
                u'Синий': STATE_ATTACK_BLUE,
                u'Желтый': STATE_ATTACK_YELLOW,
                u'Красный': STATE_ATTACK_RED,
            }
            if castle_word in word_to_state.keys():
                state = word_to_state[castle_word]
            continue

        if u'Атака на' in line and u'Лесной форт' in line:
            state = STATE_ATTACK_FOREST_FORT
            continue

        if u'Атака на' in line and u'Горный форт' in line:
            state = STATE_ATTACK_MOUNTAIN_FORT
            continue

        # match = re.match(u'.*Защита\s+своего\s+замка.*', line)
        # if match is not None:
        if u'Защита' in line and u'Черного' in line:
            state = STATE_DEFENCE
            continue

        if u'Защита' in line and u'Лесного форта' in line:
            state = STATE_DEFENCE_FOREST_FORT
            continue

        if u'Защита' in line and u'Горного форта' in line:
            state = STATE_DEFENCE_MOUNTAIN_FORT
            continue

        match = re.match(u'.*В\s+лесу.*', line)
        if match is not None:
            state = STATE_FOREST
            continue

        match = re.match(u'.*Возишься\s+с\s+КОРОВАНАМИ.*', line)
        if match is not None:
            state = STATE_KOROVANY
            continue

        match = re.match(u'.*На\s+арене.*', line)
        if match is not None:
            state = STATE_ARENA
            continue

        match = re.match(u'.*Пьешь\sв\s+таверне.*', line)
        if match is not None:
            state = STATE_TAVERN
            continue

        match = re.match(u'.*Отдых.*', line)
        if match is not None:
            state = 'rest'
            continue

    if team is None:
        team = TEAM_UNKNOWN
    if state is None:
        state = STATE_UNKNOWN

    if None in (team, gold, stamina, stamina_max, state):
        logger.warn("Can not parse hero text")
        return None

    return HeroStatus(
        team=team,
        gold=gold,
        stamina=stamina,
        stamina_max=stamina_max,
        state=state)

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
                u'В лесу ты отдохнул',
            ]
            for t in exit_options:
                if t in msg.text:
                    logger.info("wait_forest_done complete")
                    return True
        time.sleep(delay)
    logger.info('time is out; quit forest')
    return False

def wait_arena_search_done(min_to_wait):
    logger.info('wait_arena_search_done {0} minutes'.format(min_to_wait))
    sec_to_wait = min_to_wait * 60
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

def wait_any(delay=5):
    logger.info('wait any')
    sec_to_wait = 5 * 60
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
            logger.info("wait any complete")
            return True
        time.sleep(delay)
    logger.info('time is out; quit wait')
    return False


def wait_any_and_save(min_to_wait):
    logger.info('wait any and save')
    sec_to_wait = min_to_wait * 60
    delay = 10
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
            append_recipe_candidate(msg.text)
            logger.info("wait any and save complete")
            return True
        time.sleep(delay)
    logger.info('time is out; quit wait')
    return False



def do_forest():
    send_msg(QUESTS)
    wait_any()
    send_msg(FOREST_QUEST)
    wait_forest_done()

def do_attack(color):
    send_msg(ATTACK)
    wait_any()
    send_msg(color)
    wait_any()

def do_defence(target):
    send_msg(DEFENCE)
    wait_any()
    send_msg(target)
    wait_any()

def do_go():
    send_msg(GO)
    wait_any()
    wait_any()

def do_battle_step(options):
    send_msg(random.choice(options))
    sec_to_wait = 3 * 60
    delay = 3
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
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

def do_arena(min_to_wait):
    send_msg(CASTLE)
    wait_any()
    send_msg(ARENA)
    wait_any()
    send_msg(DO_SEARCH)
    res = wait_arena_search_done(min_to_wait)
    if not res:
        send_msg(CANCEL_SEARCH)
        return False

    options = HITS_DEFS
    while True:
        next_options = do_battle_step(options)
        if not next_options:
            break
        options = next_options
    return True

def do_tavern():
    send_msg(CASTLE)
    wait_any()
    send_msg(TAVERN)
    wait_any()
    send_msg(TAKE_ELL)
    wait_any_and_save(min_to_wait=2)
    wait_any_and_save(min_to_wait=10)


def get_hero_status():
    send_msg(HERO)
    sec_to_wait = 5 * 60
    delay = 3
    for i in range(sec_to_wait / delay):
        msg = get_last_chat_wars_msg()
        if msg is not None:
            return parse_hero_status(msg.text)
        time.sleep(delay)
    return None


def do_job():
    # append_recipe_candidate(TAVERN)
    # append_recipe_candidate(CASTLE)
    # return

    # for i in range(8):
    #     do_forest()
    # return

    # while True:
    #     print time_to_battle()
    #     print get_hero_status()
    #     break
    #     time.sleep(10)
    # return

    # print time_to_battle()
    # do_arena(20)
    # return 

    # for color in (BLUE, RED, WHITE, YELLOW):
    #     do_attack(color)

    # do_defence(BLACK)
    # return

    # send_msg(DEFENCE_COMMAND)
    # time.sleep(1)

    # send_msg(BACK)
    # time.sleep(1)
    # return

    # last_arena_time = None
    # # last_send_time = None
    # last_state = None

    last_arena_time = None
    last_tavern_time = None
    last_hs = None
    desired_battle_state = STATE_DEFENCE

    cry_set = True

    while True:
        msg = get_last_command_chat_msg()
        key_to_state = {
            BLUE: STATE_ATTACK_BLUE,
            RED: STATE_ATTACK_RED,
            YELLOW: STATE_ATTACK_YELLOW,
            WHITE: STATE_ATTACK_WHITE,
            DEFENCE_COMMAND: STATE_DEFENCE,
            FOREST: STATE_ATTACK_FOREST_FORT,
            MOUNTAIN: STATE_ATTACK_MOUNTAIN_FORT,
        }
        if msg is not None:
            key_count = 0
            for key in key_to_state.keys():
                if key in msg.text:
                    key_count += 1
                if ATTACK_SIGN in msg.text:
                    key_count += 1
            if key_count == 1:
                for key, state in key_to_state.iteritems():
                    if key in msg.text:
                        desired_battle_state = state

        ttb = time_to_battle()
        now = datetime.datetime.now(TZ)

        if ttb > datetime.timedelta(hours=3):
            desired_battle_state = STATE_DEFENCE

        # if ttb > datetime.timedelta(minutes=15) and ttb < datetime.timedelta(hours=2, minutes=45) and cry_set:
        #     # if (now.hour >= 19 or now.hour < 7):
        #     #     send_msg(u'Пойду напьюсь')
        #     # else:
        #     #     send_msg(u'На арену что ли сходить')
        #     send_msg(u'Follow the dark side')
        #     wait_any()
        #     cry_set = False

        if ttb > datetime.timedelta(hours=3, minutes=55):
            last_hs = None

        if ttb > datetime.timedelta(minutes=5) and ttb < datetime.timedelta(hours=3, minutes=55) and now.hour >= 6:
            msg = get_last_chat_wars_msg()
            if msg is not None:
                if GO in msg.text:
                    do_go()
                    last_hs = None

        #     # if ttb > datetime.timedelta(minutes=15) and now.hour >= 6:
        #     if ttb > datetime.timedelta(minutes=15):
        #         if last_arena_time is None or now - last_arena_time > datetime.timedelta(minutes=61):

        #             min_to_wait = 20
        #             if ttb < datetime.timedelta(minutes=35):
        #                 min_to_wait = max(1, ttb.seconds / 60 - 15)

        #             arena_result = do_arena(min_to_wait)
        #             if arena_result:
        #                 last_arena_time = datetime.datetime.now(TZ)
        #             else:
        #                 if last_arena_time is None:
        #                     # FIXME eval precise last_arena_time from history
        #                     last_arena_time = datetime.datetime.now(TZ) - datetime.timedelta(minutes=30)
        #             last_hs = None
        #             continue

        if ttb > datetime.timedelta(hours=1) and ttb < datetime.timedelta(hours=3, minutes=55):
            if last_hs is None:
                last_hs = get_hero_status()
            if last_hs is None:
                continue

            nice_hour = (8 <= now.hour < 10) or (16 <= now.hour < 18) or (21 <= now.hour < 23) or (0 <= now.hour < 2)
            if last_hs.stamina >= 10 or (nice_hour and last_hs.stamina > 0):
                do_forest()
                last_hs = None
                continue

            # if ttb > datetime.timedelta(minutes=30) and (now.hour >= 19 or now.hour < 7):
            #     if last_tavern_time is None or now - last_tavern_time > datetime.timedelta(minutes=3):
            #         if last_hs is None:
            #             last_hs = get_hero_status()
            #         if last_hs is None:
            #             continue

            #         if last_hs.gold >= 100:
            #             last_tavern_time = datetime.datetime.now(TZ)
            #             do_tavern()
            #             last_hs = None
            #             continue


        default_order_time = (now.day * 11 + now.hour * 31) % 20 + 10
        if ttb < datetime.timedelta(minutes=default_order_time):
            if last_hs is None:
                last_hs = get_hero_status()
            if last_hs is None:
                continue

            if last_hs.state != desired_battle_state:
                state_to_color = {
                    STATE_ATTACK_RED: RED,
                    STATE_ATTACK_BLUE: BLUE,
                    STATE_ATTACK_WHITE: WHITE,
                    STATE_ATTACK_YELLOW: YELLOW,
                    STATE_ATTACK_FOREST_FORT: FOREST_FORT,
                    STATE_ATTACK_MOUNTAIN_FORT: MOUNTAIN_FORT,
                }
                if desired_battle_state in state_to_color:
                    color = state_to_color[desired_battle_state]
                    do_attack(color)

                    send_msg('/on_106') # эльф меч
                    time.sleep(1)
                    send_msg('/on_150') # кинжал
                    time.sleep(1)

                    # color_to_msg = {
                    #     RED: u'В атаку на красных! За мноооой!!!',
                    #     BLUE: u'Валим синих! Урааааа!!!',
                    #     YELLOW: u'Не оставим от желтого замка камня на камне!!!',
                    #     WHITE: u'DarthVader идет на белых! Вперееед!!!',
                    # }
                    # send_msg(color_to_msg[color])
                    # wait_any()                   
                else:
                    do_defence(BLACK)
                    send_msg('/on_123') # эльф копье
                    time.sleep(1)
                    send_msg('/on_127') # мифр щит
                    time.sleep(1)
                    # send_msg(u'DarthVader прикроет родной Мордор')
                    # wait_any()
                # cry_set = True
                last_hs = None

        # time.sleep(1)
        time.sleep(random.randint(20, 40))



def setup_logger():
    hdlr = logging.StreamHandler(sys.stdout)
    # FIXME correct output taking timezone in account
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
    global sender
    sender = Sender(host="localhost", port=4458)


    send_msg("info: I'm started", SELF_PEER)
    global buddy_state
    buddy_state = BUDDY_STATE_WORKING
    try:
        while True:
            try:
                do_control_stuff()

                if buddy_state == BUDDY_STATE_WORKING:
                    do_job()
                    # for correctness if there is no loop in do_job
                    break

            except PauseException:
                buddy_state = BUDDY_STATE_PAUSED
                send_buddy_status()

            time.sleep(1)
    except:
        send_msg("info: I'm crashed", SELF_PEER)
        logger.exception('total crash')
    else:
        send_msg("info: I'm finished", SELF_PEER)
    
if __name__ == '__main__':
    main()