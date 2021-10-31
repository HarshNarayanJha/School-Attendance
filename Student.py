import os
import pickle

STUDENTS_FILENAME: str = "students.txt"
STUDENTS_PICKLE_FILENAME: str = "students_data.pickle"

class Student:
	def __init__(self, uid: int, name: str, roll: int):
		self.uid = uid
		self.name = name
		self.roll = roll

	def __repr__(self) -> str:
		return f"{self.name} <{self.uid}>, Roll No. = {self.roll}"


def read_students_data(filename: str = STUDENTS_FILENAME, force_raw: bool = False) -> "dict[int, Student]":
	out = {}

	if not force_raw:
		if os.path.exists(STUDENTS_PICKLE_FILENAME):
			with open(STUDENTS_PICKLE_FILENAME, "rb") as fp:
				out = pickle.loads(fp.read())
				return out

	with open(filename, 'r') as fp:
		lines = fp.readlines()

		for line in lines:
			lines[lines.index(line)] = line.strip()

		for line in lines:
			if line.startswith("#"): 
				continue
			
			uid, name, roll = line.split(",")
			uid = int(uid)
			roll = int(roll)

			out[uid] = Student(uid, name, roll)

	return out

def compile_students_data(data: dict, force: bool = False):
	if os.path.exists(STUDENTS_PICKLE_FILENAME):
		if force:
			os.remove(STUDENTS_PICKLE_FILENAME)
		else:
			print("Already Compiled, Skipping...")
			return

	with open(STUDENTS_PICKLE_FILENAME, "wb+") as fp:
		pickle.dump(data, fp);

if __name__ == "__main__":
	if input("Generate Pickle file (y/n): ").lower() == 'y':
		compile_students_data(read_students_data(force_raw=True), force=True)
