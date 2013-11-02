from Converter import Converter
from time import localtime, strftime
from Components.Element import cached

MONTHS = (_("January"),
          _("February"),
          _("March"),
          _("April"),
          _("May"),
          _("June"),
          _("July"),
          _("August"),
          _("September"),
          _("October"),
          _("November"),
          _("December"))

shortMONTHS = (_("Jan"),
               _("Feb"),
               _("Mar"),
               _("Apr"),
               _("May"),
               _("Jun"),
               _("Jul"),
               _("Aug"),
               _("Sep"),
               _("Oct"),
               _("Nov"),
               _("Dec"))

DAYWEEK = (_("Monday"),
           _("Tuesday"),
           _("Wednesday"),
           _("Thursday"),
           _("Friday"),
           _("Saturday"),
           _("Sunday"))

dayOfWeek = (_("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat"), _("Sun"))

class ClockToText(Converter, object):
	DEFAULT = 0
	WITH_SECONDS = 1
	IN_MINUTES = 2
	DATE = 3
	FORMAT = 4
	AS_LENGTH = 5
	TIMESTAMP = 6
	FULL = 7
	SHORT_DATE = 8
	LONG_DATE = 9
	VFD = 10
	AS_LENGTHHOURS = 11
	AS_LENGTHSECONDS = 12
	
	# add: date, date as string, weekday, ...
	# (whatever you need!)
	
	def __init__(self, type):
		Converter.__init__(self, type)
		if type == "WithSeconds":
			self.type = self.WITH_SECONDS
		elif type == "InMinutes":
			self.type = self.IN_MINUTES
		elif type == "Date":
			self.type = self.DATE
		elif type == "AsLength":
			self.type = self.AS_LENGTH
		elif type == "AsLengthHours":
			self.type = self.AS_LENGTHHOURS
		elif type == "AsLengthSeconds":
			self.type = self.AS_LENGTHSECONDS
		elif type == "Timestamp":	
			self.type = self.TIMESTAMP
		elif type == "Full":
			self.type = self.FULL
		elif type == "ShortDate":
			self.type = self.SHORT_DATE
		elif type == "LongDate":
			self.type = self.LONG_DATE
		elif type == "VFD":
			self.type = self.VFD
		elif "Format" in type:
			self.type = self.FORMAT
			self.fmt_string = type[7:]
		else:
			self.type = self.DEFAULT

	@cached
	def getText(self):
		time = self.source.time
		if time is None:
			return ""

		# handle durations
		if self.type == self.IN_MINUTES:
			return "%d min" % (time / 60)
		elif self.type == self.AS_LENGTH:
			if time < 0:
				return ""
			return "%d:%02d" % (time / 60, time % 60)
		elif self.type == self.AS_LENGTHHOURS:
			if time < 0:
				return ""
			return "%d:%02d" % (time / 3600, time / 60 % 60)
		elif self.type == self.AS_LENGTHSECONDS:
			if time < 0:
				return ""
			return "%d:%02d:%02d" \
			 % (time / 3600, time / 60 % 60, time % 60)
		elif self.type == self.TIMESTAMP:
			return str(time)
		
		t = localtime(time)
		
		if self.type == self.WITH_SECONDS:
			# TRANSLATORS: full time representation hour:minute:seconds 
			return _("%2d:%02d:%02d") % (t.tm_hour, t.tm_min, t.tm_sec)
		elif self.type == self.DEFAULT:
			# TRANSLATORS: short time representation hour:minute
			return _("%2d:%02d") % (t.tm_hour, t.tm_min)
		elif self.type == self.DATE:
			# TRANSLATORS: full date representation dayname daynum monthname year in strftime() format! See 'man strftime'
			d = _(strftime("%A",t)) + " " + str(t[2]) + " " + MONTHS[t[1]-1] + " " + str(t[0])
		elif self.type == self.FULL:
			# TRANSLATORS: long date representation short dayname daynum short monthname hour:minute in strftime() format! See 'man strftime'
			d = dayOfWeek[t[6]] + _(" %d/%d  %2d:%02d") % (t[2],t[1], t.tm_hour, t.tm_min)
		elif self.type == self.SHORT_DATE:
			# TRANSLATORS: short date representation short dayname daynum short monthname in strftime() format! See 'man strftime'
			d = dayOfWeek[t[6]] + _(" %d/%d") % (t[2], t[1])
		elif self.type == self.LONG_DATE:
			# TRANSLATORS: long date representations dayname daynum monthname in strftime() format! See 'man strftime'
			d = dayOfWeek[t[6]] + " " + str(t[2]) + " " + MONTHS[t[1]-1]
		elif self.type == self.VFD:
			# TRANSLATORS: VFD hour:minute daynum short monthname in strftime() format! See 'man strftime'
			d = _("%2d:%02d %d/%d") % (t.tm_hour, t.tm_min, t[2], t[1])
		elif self.type == self.FORMAT:
			spos = self.fmt_string.find('%')
			self.fmt_string = self.fmt_string.replace('%A',_(DAYWEEK[t.tm_wday]))
			self.fmt_string = self.fmt_string.replace('%B',_(MONTHS[t.tm_mon-1]))
			self.fmt_string = self.fmt_string.replace('%a',_(dayOfWeek[t.tm_wday]))
			self.fmt_string = self.fmt_string.replace('%b',_(shortMONTHS[t.tm_mon-1]))
			if spos > 0:
				d = str(self.fmt_string[:spos]+self.fmt_string[spos:])
			else:
				d = self.fmt_string
		else:
			return "???"
		return strftime(d, t)

	text = property(getText)
