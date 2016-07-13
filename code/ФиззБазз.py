#!/usr/bin/env python3
# -*- coding: utf-8 -*-


def главный():
    for номер in range(1, 30):
        if номер % 3 == 0 and номер % 5 == 0:
            print("ФиззБазз")
        elif номер % 3 == 0:
            print("Физз")
        elif номер % 5 == 0:
            print("Базз")
        else:
            print(номер)


if __name__ == '__main__':
    главный()
