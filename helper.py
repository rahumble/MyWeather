import subprocess

base = "http://openweathermap.org/img/w/"

numbers = ['01','02','03','04','09','10','11','13','50']

def downloadFiles():
	for i, n in enumerate(numbers):
		path = base + n
		print "Getting pictures:"
		subprocess.call(["wget", path + "d.png"])
		subprocess.call(["wget", path + "n.png"])

#downloadFiles();

def addMedia():
	for _,t in enumerate(["d","n"]):
		for _, n in enumerate(numbers):
			print "{"
			print "\"type\": \"png\","
			print "\"name\": \"" + n + t + "\","
			print "\"file\": \"img/" + n + t+ ".png\""
			print "},"

#addMedia();

def printResources():
	for _,t in enumerate(["d","n"]):
		for _, n in enumerate(numbers):
			print "RESOURCE_ID_" + n + t + ","

#printResources();

def printCases():
	count = 0
	for _,t in enumerate(["d","n"]):
		for _, n in enumerate(numbers):
			print "case  \"" + n + t + "\":"
			print "\treturn " + str(count) + ";"
			print "\tbreak;"
			count += 1

printCases();