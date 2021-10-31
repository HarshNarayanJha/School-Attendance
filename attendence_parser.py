import datetime
import os
import pandas as pd
import pickle

from Student import read_students_data, Student

all_students: "dict[int, Student]" = read_students_data()

attendance: "dict[int, tuple]" = {}

#ATTENDANCE_FILENAME: str = f"Attendance_{datetime.date.today().year}_{datetime.date.today().month}_{datetime.date.today().day}.txt"
ATTENDANCE_FILENAME: str = "attendance_2021_10_28.txt"		
	
with open(ATTENDANCE_FILENAME, 'r') as fp:
	
	lines = fp.readlines()
	
	for line in lines:
		lines[lines.index(line)] = line.strip()
		
	for line in lines:
		if line.startswith("#"):
			continue
			
		uid, time, onTime = line.split(",")
		
		uid = int(uid)
		time = datetime.time.fromisoformat(time)
		onTime = "On Time" if onTime.lower() == "true" else "Late"
		
		if uid in all_students:    		
			# row = (uid, all_students[uid].name, all_students[uid].roll, time, onTime)
			row = (all_students[uid].name, all_students[uid].roll, time, onTime)
			#print(*row)
			
			attendance[uid] = row

# for i in all_students:
# 	print(all_students[i])

print("\n\n** Attendance **\n")

data_dict = {"Roll No.": [], "Name": [], "Present": [], "Time": [], "Was On Time": []}


# for i in attendance:
# 	print(f"UID: {i}")
# 	print(f"Name: {attendance[i][0]}")
# 	print(f"Roll No.: {attendance[i][1]}")
# 	print(f"Time: {attendance[i][2]}")
# 	print(f"Was On Time: {attendance[i][3]}")
# 	print()

OUTPUT_ONLY_PRESENT: bool = True

for i in all_students:

	if OUTPUT_ONLY_PRESENT:
		if i in attendance:
			data_dict["Roll No."].append(all_students[i].roll)
			data_dict["Name"].append(all_students[i].name)
			data_dict["Present"].append("Present")
			data_dict["Time"].append(attendance[i][2])
			data_dict["Was On Time"].append(attendance[i][3])
	else:
		data_dict["Roll No."].append(all_students[i].roll)
		data_dict["Name"].append(all_students[i].name)

		if i in attendance:		# means that UID-i is present
			data_dict["Present"].append("Present")
			data_dict["Time"].append(attendance[i][2])
			data_dict["Was On Time"].append(attendance[i][3])
		else:
			data_dict["Present"].append("Absent")
			data_dict["Time"].append(None)
			data_dict["Was On Time"].append(None)
	
df = pd.DataFrame(data_dict).set_index("Roll No.")
print(df)

writer = pd.ExcelWriter('Attendance.xlsx', engine='xlsxwriter')
df.to_excel(writer, sheet_name='Sheet1')
writer.save()

print(f"\nTotal Attendances: {len(attendance)}")