# -*- coding: utf-8 -*-
#
# Extended Converter for VFD displays
# By Taapat 2015
#
# Based on extended ServiceName Converter ServiceName2
# Coded by vlamo (c) 2011
# Support: http://dream.altmaster.net/
#
# Translit from pytils - russian-specific string utils
# Copyright (C) 2006-2009  Yury Yurevich
# http://pyobject.ru/projects/pytils/
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, version 2
# of the License
#

from datetime import datetime
from Poll import Poll
from Components.Converter.Converter import Converter
from enigma import iPlayableServicePtr
from Components.Element import cached


class VfdDisplay(Poll, Converter, object):
	def __init__(self, type):
		Converter.__init__(self, type)
		Poll.__init__(self)
		self.num = None
		self.name = None
		self.index = -1
		self.type = []
		for x in type.split(';'):
			self.type.append(x)
		self.lenght = len(self.type)-1
		if self.lenght >= 1 and self.type[self.lenght].isdigit():
			self.poll_interval = int(self.type[self.lenght])*1000
			self.type.remove(self.type[self.lenght])
			self.lenght -= 1
		else:
			self.poll_interval = 10000
		self.poll_enabled = True

	def translify(self, tr_string):
		TRANSTABLE = (
			("Щ", "Shс"),
			("Ё", "Yo"),
			("Ж", "Zh"),
			("Ц", "Ts"),
			("Ч", "Ch"),
			("Ш", "Sh"),
			("Ы", "Y"),
			("Ю", "U"),
			("Я", "Ya"),
			("А", "A"),
			("Б", "B"),
			("В", "V"),
			("Г", "G"),
			("Д", "D"),
			("Е", "E"),
			("З", "3"),
			("И", "I"),
			("Й", "J"),
			("К", "K"),
			("Л", "L"),
			("М", "M"),
			("Н", "H"),
			("О", "O"),
			("П", "P"),
			("Р", "R"),
			("С", "C"),
			("Т", "T"),
			("У", "U"),
			("Ф", "F"),
			("Х", "H"),
			("Э", "E"),
			("Ъ", ""),
			("Ь", ""),
			("щ", "shc"),
			("ё", "yo"),
			("ж", "zh"),
			("ц", "ts"),
			("ч", "ch"),
			("ш", "sh"),
			("ы", "y"),
			("ю", "u"),
			("я", "ya"),
			("а", "a"),
			("б", "b"),
			("в", "v"),
			("г", "g"),
			("д", "d"),
			("е", "e"),
			("з", "z"),
			("и", "i"),
			("й", "j"),
			("к", "k"),
			("л", "l"),
			("м", "m"),
			("н", "n"),
			("о", "o"),
			("п", "p"),
			("р", "r"),
			("с", "c"),
			("т", "t"),
			("у", "u"),
			("ф", "f"),
			("х", "h"),
			("э", "e"),
			("ъ", ""),
			("ь", ""))
		for symb_in, symb_out in TRANSTABLE:
			tr_string = tr_string.replace(symb_in, symb_out)
		return tr_string

	@cached
	def getText(self):
		if self.index < self.lenght:
			self.index += 1
		else:
			self.index = 0
		if self.type[self.index] == "Number":
			if self.num:
				return self.num
			else:
				try:
					service = self.source.serviceref
					if service:
						self.num = '%04d' % service.getChannelNum()
					else:
						self.num = None
				except:
					pass
				if self.num:
					return self.num
		elif self.type[self.index] == "Time" or self.type[self.index] == "Clock":
			return datetime.today().strftime("%H%M")
		elif self.type[self.index] == "Name":
			if self.name:
				return self.name
			else:
				service = self.source.service
				if isinstance(service, iPlayableServicePtr):
					info = service and service.info()
					ref = None
				else: # reference
					info = service and self.source.info
					ref = service
				if info:
					if ref:
						self.name = info.getName(ref) or ""
					else:
						self.name = info.getName() or ""
					self.name = self.translify(self.name.replace("\xc2\x86", "").replace("\xc2\x87", ""))
				if self.name:
					return self.name
		else:
			return str(self.type[self.index])

	text = property(getText)

	def changed(self, what):
		if what[0] is self.CHANGED_SPECIFIC:
			self.num = None
			self.name = None
			self.index = -1
			Converter.changed(self, what)
		elif what[0] is self.CHANGED_POLL:
			self.downstream_elements.changed(what)

