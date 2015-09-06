import subprocess

#names = ["sun", "rain", "cloud", "snow"]
names = ["sun"]

HEIGHT = 28
WIDTH = 28

def resizeFiles():
	for _, n in enumerate(names):
		print str(WIDTH) + "x" + str(HEIGHT)

		subprocess.call(["convert", "originalImages/" + n + ".png",
			"-adaptive-resize", str(WIDTH) + "x" + str(HEIGHT),
			"-fill", "#000000",
			"-opaque", "none",
			"-type", "Grayscale",
			"-colorspace", "Gray",
			"-black-threshold", "30%",
			"-white-threshold", "70%",
			"-ordered-dither", "2x1",	
			"-colors", "2",
			"-depth", "1",
			"-define", "png:compression-level=9",
			"-define", "png:compression-strategy=0",
			"-define", "png:exclude-chunk=all",
			"resources/img/menu_icon	.png"])
			#"resources/img/" + n + "_small.png"])

resizeFiles();