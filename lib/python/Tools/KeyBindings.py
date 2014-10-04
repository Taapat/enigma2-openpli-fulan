
keyBindings = { }

from keyids import KEYIDS
from Components.config import config
from Components.RcModel import rc_model

def addKeyBinding(domain, key, context, action, flags):
	keyBindings.setdefault((context, action), []).append((key, domain, flags))

# returns a list of (key, flags) for a specified action
def queryKeyBinding(context, action):
	if (context, action) in keyBindings:
		return [(x[0], x[2]) for x in keyBindings[(context, action)]]
	else:
		return [ ]

def getKeyDescription(key):
	for key_name, key_id in KEYIDS.items():
		if key_id != key: continue
		if key_name.startswith("KEY_"):
			return (key_name[4:],)
		return

def removeKeyBindings(domain):
	# remove all entries of domain 'domain'
	for x in keyBindings:
		keyBindings[x] = filter(lambda e: e[1] != domain, keyBindings[x])
