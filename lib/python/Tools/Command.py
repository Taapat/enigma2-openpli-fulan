from os import stat as os_stat, path as os_path, system
import os

def command(comandline, strip=1):
	comandline = comandline + " >/tmp/command.txt"
	os.system(comandline)
	text = ""
	if os.path.exists("/tmp/command.txt") is True:
		file = open("/tmp/command.txt", "r")
		if strip == 1:
			for line in file:
				text = text + line.strip() + '\n'
		else:
			for line in file:
				text = text + line
				if text[-1:] != '\n': text = text + "\n"
		file.close
	if text[-1:] == '\n': text = text[:-1]
	comandline = text
	os.system("rm /tmp/command.txt")
	return comandline

