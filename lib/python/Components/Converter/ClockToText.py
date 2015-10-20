from time import localtime, strftime
from Converter import Converter
from Components.Element import cached

class ClockToText(Converter, object):
	def __init__(self, type):
		Converter.__init__(self, type)

		self.type = type
		self.fix = ""
		if ";" in type:
			type, self.fix = type.split(";")
		if "Format" in type:
			self.fmt_string = type[7:]

	@cached
	def getText(self):
		time = self.source.time
		if time is None:
			return ""

		# add/remove 1st space
		def fix_space(string):
			if "Proportional" in self.fix and t.tm_hour < 10:
				return " " + string
			if "NoSpace" in self.fix:
				return string.lstrip(' ')
			return string

		# handle durations
		if self.type == "InMinutes":
			return _("%d min") % (time / 60)
		elif self.type == "AsLength":
			if time < 0:
				return ""
			return "%d:%02d" % (time / 60, time % 60)
		elif self.type == "AsLengthSeconds":
			if time < 0:
				return ""
			return "%d:%02d" % (time / 3600, time / 60 % 60)
		elif self.type == "AsLengthSeconds":
			if time < 0:
				return ""
			return "%d:%02d:%02d" % (time / 3600, time / 60 % 60, time % 60)
		elif self.type == "Timestamp":
			return str(time)

		t = localtime(time)

		if "Format" in self.type:
			d = self.fmt_string
		elif self.type == "WithSeconds":
			# TRANSLATORS: full time representation hour:minute:seconds
			return fix_space(_("%2d:%02d:%02d") % (t.tm_hour, t.tm_min, t.tm_sec))
		elif self.type == "Date":
			# TRANSLATORS: full date representation dayname daynum monthname year in strftime() format! See 'man strftime'
			d = _("%A %e %B %Y")
		elif self.type == "Full":
			# TRANSLATORS: long date representation short dayname daynum short monthname hour:minute in strftime() format! See 'man strftime'
			d = _("%a %e/%m  %-H:%M")
		elif self.type == "ShortDate":
			# TRANSLATORS: short date representation short dayname daynum short monthname in strftime() format! See 'man strftime'
			d = _("%a %e/%m")
		elif self.type == "LongDate":
			# TRANSLATORS: long date representations dayname daynum monthname in strftime() format! See 'man strftime'
			d = _("%A %e %B")
		elif self.type == "VFD":
			# VFD hour minute
			return "%02d%02d" % (t.tm_hour, t.tm_min)
		else:
			# TRANSLATORS: default time format hour:minute
			return fix_space(_("%2d:%02d") % (t.tm_hour, t.tm_min))
		return strftime(d, t)

	text = property(getText)

