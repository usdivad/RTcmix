rtsetparams(44100, 1)
makegen(1, 10, 1000, 1, 0.3, 0.2)
makegen(2, 7, 1000, 0, 50, 1, 900, 1, 50, 0)

start = 0.0
freq = 149.0

for (i = 0; i < 40; i = i+1) {
	WAVETABLE(start, 40-start, 500, freq)
	start = start + 1.0
	freq = freq + 25
	}
