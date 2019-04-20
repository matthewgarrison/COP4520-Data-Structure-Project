import re
import sys
from pathlib import Path
import matplotlib.pyplot as pyplot
# from matplotlib.ticket import MaxNLocator

OUTPUT_FILE_FORMAT = "{0}.png"
LINE_FORMAT = (
	r"^(?P<num_threads>[0-9]) (?P<execution_time>[0-9]+(\.[0-9]*)?)\n$"
)
DETAILS_FORMAT = (
	r"(?P<percent_pushback>[0-9]*\%) pushback, (?P<percent_popback>[0-9]*\%) popback, (?P<percent_read>[0-9]*\%) read, (?P<percent_write>[0-9]*\%) write\n$"
)
HEADER_FORMAT = (
	"{0}% push, {1}% pop, {2}%read, {3}% write"
)

x = 0

def panic(message):
	print(message)
	exit(1)
try:
	thread_counts = []
	execution_times = []

	with open(sys.argv[1], "r") as input_file:
		for j in range(3):
			detail_line = input_file.readline()
			detail_line_match = re.match(DETAILS_FORMAT, detail_line)
			if detail_line_match:
				print("hello")
				pushback_ratio = detail_line_match.group("percent_pushback")
				print('pushback_ratio', pushback_ratio)
				popback_ratio = detail_line_match.group("percent_popback")
				print('popback_ratio', popback_ratio)
				read_ratio = detail_line_match.group("percent_read")
				print('read_ratio', read_ratio)
				write_ratio = detail_line_match.group("percent_write")
				print('write_ratio', write_ratio)
			for i in range(4):
				print(i)
				line = input_file.readline()
				line_match = re.match(LINE_FORMAT, line)
				if line_match:
					thread_counts.append(line_match.group("num_threads"))
					execution_times.append(line_match.group("execution_time"))
				else:
					raise SyntaxError
			axis = pyplot.figure().gca()

			pyplot.plot(thread_counts, execution_times)
			pyplot.title(HEADER_FORMAT.format(pushback_ratio, popback_ratio, read_ratio, write_ratio))
			pyplot.xlabel("Thread count")
			pyplot.ylabel("Execution in seconds")

			file_stem = str(Path(sys.argv[1]).stem) + str(x)
			pyplot.savefig(OUTPUT_FILE_FORMAT.format(file_stem))
			x += 1
			thread_counts.clear()
			execution_times.clear()
except SyntaxError:
	panic("Data file's syntax is incorrect")
except Exception:
	panic("Something went wrong!")