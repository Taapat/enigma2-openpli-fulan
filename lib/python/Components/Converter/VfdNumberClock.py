#
# Simple Converter for VFD displays
# By Taapat 2015, thx 2boom
#

from datetime import datetime
from Poll import Poll
from Components.Converter.Converter import Converter
from Components.Element import cached


class VfdNumberClock(Poll, Converter, object):
	def __init__(self, type):
		Converter.__init__(self, type)
		Poll.__init__(self)
		self.type = type and str(type)
		self.index = False
		self.num = None
		self.poll_interval = 10000
		self.poll_enabled = True

	@cached
	def getText(self):
		if self.index:
			self.index = False
			return datetime.today().strftime('%H%M')
		else:
			self.index = True
			if self.type:
				return self.type
			elif self.num:
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

	text = property(getText)

	def changed(self, what):
		if what[0] is self.CHANGED_SPECIFIC:
			self.index = False
			self.num = None
			Converter.changed(self, what)
		elif what[0] is self.CHANGED_POLL:
			self.downstream_elements.changed(what)

