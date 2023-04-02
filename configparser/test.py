# -*-coding:utf-8 -*-

from par import ConfigParser

conf = ConfigParser()
print('读取成功：', conf.read('./copier.ini', 'GBK'))
print(conf.get('copier', 'powerpointpath'))
print(conf.get('copier', 'copypath'))
print(conf.get('copier', 'ignoredisk'))
print(conf.items('copier'))
