from Converter import Converter
from time import localtime, strftime
from Components.Element import cached

class ClockToText(Converter, object):
	def __init__(self, type):
		Converter.__init__(self, type)
		self.type = type
		if "Format" in type:
			self.fmt_string = type[7:]

	@cached
	def getText(self):
		time = self.source.time
		if time is None:
			return ""

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

		if self.type == "WithSeconds":
			# TRANSLATORS: full time representation hour:minute:seconds
			return _("%2d:%02d:%02d") % (t.tm_hour, t.tm_min, t.tm_sec)
		elif self.type == "Date":
			# TRANSLATORS: full date representation dayname daynum monthname year in strftime() format! See 'man strftime'
			d = _(strftime("%A",t)) + " " + str(t[2]) + " " + MONTHS[t[1]-1] + " " + str(t[0])
		elif self.type == "Full":
			# TRANSLATORS: long date representation short dayname daynum short monthname hour:minute in strftime() format! See 'man strftime'
			d = dayOfWeek[t[6]] + _(" %d/%d  %2d:%02d") % (t[2],t[1], t.tm_hour, t.tm_min)
		elif self.type == "ShortDate":
			# TRANSLATORS: short date representation short dayname daynum short monthname in strftime() format! See 'man strftime'
			d = dayOfWeek[t[6]] + _(" %d/%d") % (t[2], t[1])
		elif self.type == "LongDate":
			# TRANSLATORS: long date representations dayname daynum monthname in strftime() format! See 'man strftime'
			d = dayOfWeek[t[6]] + " " + str(t[2]) + " " + MONTHS[t[1]-1]
		elif self.type == "VFD":
			# TRANSLATORS: VFD hour:minute in strftime() format! See 'man strftime'
			d = "%02d%02d" % (t.tm_hour, t.tm_min)
		elif "Format" in self.type:
			self.fmt_string = self.fmt_string.replace('%A',_(strftime("%A", t)))
			self.fmt_string = self.fmt_string.replace('%B',_(strftime("%B", t)))
			self.fmt_string = self.fmt_string.replace('%a',_(strftime("%a", t)))
			self.fmt_string = self.fmt_string.replace('%b',_(strftime("%b", t)))
			d = self.fmt_string
		else:
			# Default
			return _("%2d:%02d") % (t.tm_hour, t.tm_min)
		return strftime(d, t)

	text = property(getText)
