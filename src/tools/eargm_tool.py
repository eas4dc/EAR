#!/usr/bin/env python3

import re

print("")
print("=========================== This tool is used to automatically build the EAR configuration file ===========================")
print("")

print("============================= Cluster Structure Data =============================")
print("")
print("To configure energy or power capping we need information about the cluster structure.")
print("")

clusterSize = input("How many types of nodes/partitions does the cluster contain (for example, CPU-only and CPU+2GPUs): ")
while clusterSize == '' or not(clusterSize.strip().isdigit()):
	clusterSize = input("How many types of nodes/partitions does the cluster contain: ")

clusterTypeName = input("Would you like to name the cluster type of nodes/partitions [Y/n]: ")
if clusterTypeName == '':
	clusterTypeName = 'y'
while not(clusterTypeName in ['y', 'Y', 'n', 'N']) or clusterTypeName.strip().isdigit():
	if clusterTypeName == '':
		clusterTypeName = 'y'
		break
	clusterTypeName = input("[Y/n]: ")

namesList = []
if clusterTypeName in ['y', 'Y']:
	names = input("Enter the names (use comma separated format)(i.e. type1,type2 or CPU-only,CPU-GPU): ")
	pattern1 = re.compile(r"^(\w+)(,\s*\w+)*$")
	pattern2 = re.compile(r"^(\w+-\w+)(,\s*\w+-\w+)*$")
	while names == '' or len(names.split(',')) != int(clusterSize):
		names = input("Enter a valid names format: ")
	namesList = (names.replace(" ", "")).split(",")

if clusterTypeName in ['n', 'N']:
	print("The default naming format will be used, like: type1, type2,....,typeN")
	for i in range(1, int(clusterSize)+1, 1):
		namesList.append("type" + str(i))
print("")

nodesNumList = []
maxPowerList = []
averagePowerList = []
for i in namesList:
	print("Configuring type of {}".format(i))
	nodesNum = input("How many nodes of this types are there: ")
	while nodesNum == '' or not(nodesNum.strip().isdigit()):
		nodesNum = input("Enter a valid number of nodes: ")
	nodesNumList.append(nodesNum)
	maxPower = input("What is the max power consumption (in Watts): ")
	while maxPower == '' or not(maxPower.strip().isdigit()):
		maxPower = input("Enter a valid max power consumption (in Watts): ")
	maxPowerList.append(maxPower)
	averagePower = input("What is the average power consumption (in Watts): ")
	while averagePower == '' or not(averagePower.strip().isdigit()):
		averagePower = input("Enter a valid average power consumption (in Watts): ")
	averagePowerList.append(averagePower)
	print("")
print("")

print("============================= EARGM configuration =============================")
print("")

print("==========================")
print("= POWERCAP Configuration =")
print("==========================")
print("")

powerCap = input("Would you like to configure power capping [Y/n]: ")
if powerCap == '':
	powerCap = 'y'
while not(powerCap in ['y', 'Y', 'n', 'N']) or powerCap.strip().isdigit():
	if powerCap == '':
		powerCap = 'y'
		break
	powerCap = input("[Y/n]: ")

if powerCap in ['y', 'Y']:
	print("General powercap configuration:")

	EARGMPowerCapMode = input("What powercap mode should be used (monitor/0-hard powercap/1-soft powercap/2) [0/1/2]: ")
	while EARGMPowerCapMode == '' or not(EARGMPowerCapMode.strip().isdigit()) or not(EARGMPowerCapMode in ['0', '1', '2']):
		EARGMPowerCapMode = input("[0/1/2]: ")

	EARGMPowerPeriod = input("How often (in seconds) should the EARGMs ask the nodes for their power status [120]: ")
	if EARGMPowerPeriod == '':
		EARGMPowerPeriod = 120
	while not(str(EARGMPowerPeriod).strip().isdigit()):
		EARGMPowerPeriod = input("Enter a valid period (in seconds) [120]: ")
		if EARGMPowerPeriod == '':
			EARGMPowerPeriod = 120

	powerLimit = input("Do you want to manage power limit for each partition independently [y/n]: ")
	while not(powerLimit in ['y', 'Y', 'n', 'N']) or powerLimit.strip().isdigit():
		powerLimit = input("[y/n]: ")

	eargmIdList = []
	eargmTypeList = []
	eargmPowerList = []
	eargmHostnameList = []
	eargmPortList = []

	eargmS = 0
	nodesNum = 0
	for i in nodesNumList:
		num = 0
		if int(i) > 1000:
			num = round(int(i)/1000)
		eargmS = int(clusterSize) + num
		nodesNum = nodesNum + int(i)

	power = []
	for (i, j) in zip(nodesNumList, maxPowerList):
		val = int(i) * int(j)
		power.append(val)

	if powerLimit in ['y', 'Y']:
		eargmNum = input("How many EARGMs would you like to configure (Given {} nodes and {} partitions, we recommend {} EARGMs)? [{}]: ".format(str(nodesNum), str(clusterSize), str(eargmS), str(eargmS)))
		if eargmNum == '':
			eargmNum = eargmS
		while not(str(eargmNum).strip().isdigit()):
			eargmNum = input("Enter a valid EARGMs number, we recommend {} EARGMs)? [{}]: ".format(eargmS, eargmS))
		print("")

		if int(eargmNum) == 1:
			print("Configuring EARGM with ID 1")

			eargmIdList.append(1)
			eargmTypeList.append(0)

			eargmPower = input("What is this EARGM's power budget [we suggest a budget of {}W]: ".format(sum(power)))
			if eargmPower == '':
				eargmPower = sum(power)
			while not(str(eargmPower).strip().isdigit()):
				eargmPower = input("Enter a valid EARGM's power budget [we suggest a budget of {}W]: ".format(sum(power)))
				if eargmPower == '':
					eargmPower = sum(power)
			eargmPowerList.append(int(eargmPower))

			eargmHostname = input("What is the hostname where the EARGM will be running [nodename]: ")
			if eargmHostname == '':
				eargmHostname = 'nodename'
			while eargmHostname.strip().isdigit():
				eargmHostname = input("Enter a valid hostname [nodename]: ")
				if eargmHostname == '':
					eargmHostname = 'nodename'
			eargmHostnameList.append(eargmHostname)

			eargmPort = input("What port will this EARGM use for remote connections [50100]: ")
			if eargmPort == '':
				eargmPort = '50100'
			while not(eargmPort.strip().isdigit()):
				eargmPort = input("Enter a valid port number [50100]: ")
				if eargmPort == '':
					eargmPort = '50100'
			eargmPortList.append(int(eargmPort))
			print("")

		if int(eargmNum) > 1:
			for i in range(1, int(eargmNum)+1):
				print("Configuring EARGM with ID {}: ".format(i))

				eargmIdList.append(int(i))

				eargmType = input("What type of nodes will it control [{}]: ".format(','.join(map(str, namesList))))
				while eargmType == '' or eargmType.strip().isdigit():
					eargmType = input("Enter a valid type of nodes [{}]: ".format(','.join(map(str, namesList))))
				eargmTypeList.append(eargmType)

				index = namesList.index(eargmType)
				eargmPowerSug = int(nodesNumList[index]) * int(maxPowerList[index])
				eargmPower = input("What is this EARGM's power budget [we suggest a budget of {}W]: ".format(eargmPowerSug))
				if eargmPower == '':
					eargmPower = eargmPowerSug
				while not(str(eargmPower).strip().isdigit()):
					eargmPower = input("Enter a valid power budget [we suggest a budget of {}W]: ".format(eargmPowerSug))
					if eargmPower == '':
						eargmPower = eargmPowerSug
				eargmPowerList.append(int(eargmPower))

				eargmHostname = input("What is the hostname where the EARGM will be running [nodename]: ")
				if eargmHostname == '':
					eargmHostname = 'nodename'
				while eargmHostname.strip().isdigit():
					eargmHostname = input("Enter a valid hostname [nodename]: ")
					if eargmHostname == '':
						eargmHostname = 'nodename'
				eargmHostnameList.append(eargmHostname)

				eargmPort = input("What port will this EARGM use for remote connections [50100]: ")
				if eargmPort == '':
					eargmPort = '50100'
				while not(eargmPort.strip().isdigit()):
					eargmPort = input("Enter a valid port number [50100]: ")
					if eargmPort == '':
						eargmPort = '50100'
				eargmPortList.append(int(eargmPort))
				print("")

	if powerLimit in ['n', 'N']:
		print("Configuring EARGM with ID 1")

		eargmIdList.append(1)
		eargmTypeList.append(0)

		eargmPower = input("What is this EARGM's power budget [we suggest a budget of {}W]: ".format(sum(power)))
		if eargmPower == '':
			eargmPower = sum(power)
		while not(str(eargmPower).strip().isdigit()):
			eargmPower = input("Enter a valid EARGM's power budget [we suggest a budget of {}W]: ".format(sum(power)))
			if eargmPower == '':
				eargmPower = sum(power)
		eargmPowerList.append(int(eargmPower))

		eargmHostname = input("What is the hostname where the EARGM will be running [nodename]: ")
		if eargmHostname == '':
			eargmHostname = 'nodename'
		while eargmHostname.strip().isdigit():
			eargmHostname = input("Enter a valid hostname [nodename]: ")
			if eargmHostname == '':
				eargmHostname = 'nodename'
		eargmHostnameList.append(eargmHostname)

		eargmPort = input("What port will this EARGM use for remote connections [50100]: ")
		if eargmPort == '':
			eargmPort = '50100'
		while not(eargmPort.strip().isdigit()):
			eargmPort = input("Enter a valid port number [50100]: ")
			if eargmPort == '':
				eargmPort = '50100'
		eargmPortList.append(int(eargmPort))
		print("")

	#print(eargmIdList)
	#print(eargmTypeList)
	#print(eargmPowerList)
	#print(eargmHostnameList)
	#print(eargmPortList)

	EARGMPowerCapSuspendConfig = input("Do you want to set up a command when the power consumption goes above a certain percentage of the total [Y/n]: ")
	if EARGMPowerCapSuspendConfig == '':
		EARGMPowerCapSuspendConfig = 'y'
	while not(EARGMPowerCapSuspendConfig in ['y', 'Y', 'n', 'N']) or EARGMPowerCapSuspendConfig.strip().isdigit():
		if EARGMPowerCapSuspendConfig == '':
			EARGMPowerCapSuspendConfig = 'y'
			break
		EARGMPowerCapSuspendConfig = input("[Y/n]: ")

	if EARGMPowerCapSuspendConfig in ['y', 'Y']:
		EARGMPowerCapSuspendLimit = input("What percentage would trigger the command [Current power budgets: [{}]] [85]: ".format(", ".join(str(i) for i in eargmPowerList)))
		if EARGMPowerCapSuspendLimit == '':
			EARGMPowerCapSuspendLimit = '85'
		while not(EARGMPowerCapSuspendLimit.strip().isdigit()):
			EARGMPowerCapSuspendLimit = input("Enter a valid power percentage [85]: ")
			if EARGMPowerCapSuspendLimit == '':
				EARGMPowerCapSuspendLimit = '85'

		EARGMPowerCapSuspendAction = input("Please introduce the absolute path to the command (It will be executed with the following arguments\ncommand_name current_power current_limit total_idle_nodes total_idle_power: ")
		while EARGMPowerCapSuspendAction == '' or EARGMPowerCapSuspendAction.strip().isdigit():
			EARGMPowerCapSuspendAction = input("Enter a valid command: ")
		print("")

	if EARGMPowerCapSuspendConfig in ['n', 'N']:
		EARGMPowerCapSuspendLimit = 0
		EARGMPowerCapSuspendAction = "no_action"
		print("")

	EARGMPowerCapResumeConfig = input("Do you want to set up a command when the power consumption goes below a certain percentage of the total [Y/n]: ")
	if EARGMPowerCapResumeConfig == '':
		EARGMPowerCapResumeConfig = 'y'
	while not(EARGMPowerCapResumeConfig in ['y', 'Y', 'n', 'N']) or EARGMPowerCapResumeConfig.strip().isdigit():
		if EARGMPowerCapResumeConfig == '':
			EARGMPowerCapResumeConfig = 'y'
			break
		EARGMPowerCapResumeConfig = input("[Y/n]: ")

	if EARGMPowerCapResumeConfig in ['y', 'Y']:
		EARGMPowerCapResumeLimit = input("What percentage would trigger the command [Current power budgets: [{}]] [40]: ".format(", ".join(str(i) for i in eargmPowerList)))
		if EARGMPowerCapResumeLimit == '':
			EARGMPowerCapResumeLimit = '40'
		while not(EARGMPowerCapResumeLimit.strip().isdigit()):
			EARGMPowerCapResumeLimit = input("Enter a valid power percentage [40]: ")
			if EARGMPowerCapResumeLimit == '':
				EARGMPowerCapResumeLimit = '40'

		EARGMPowerCapResumeAction = input("\nPlease introduce the absolute path to the command (It will be executed with the following arguments\ncommand_name current_power current_limit total_idle_nodes total_idle_power: ")
		while EARGMPowerCapResumeAction == '' or EARGMPowerCapResumeAction.strip().isdigit():
			EARGMPowerCapResumeAction = input("Enter a valid command: ")
		print("")

	if EARGMPowerCapResumeConfig in ['n', 'N']:
		EARGMPowerCapResumeLimit = 0
		EARGMPowerCapResumeAction = "no_action"
		print("")

	
	if int(eargmNum) > 1:
		metaEargm = input("Would you like to allow power budget redistribution between EARGMs? [Y/n]: ")
		if metaEargm == '':
			metaEargm = 'Y'
		while not(metaEargm in ['y', 'Y', 'n', 'N']) or metaEargm.strip().isdigit():
			metaEargm = input("[Y/n]: ")
			if metaEargm == '':
				metaEargm = 'Y'

		if metaEargm in ['y', 'Y']:
			metaEargmNum = input("How many meta-EARGMs do you want to configure: ")
			while metaEargmNum == '' or not(metaEargmNum.strip().isdigit()):
				metaEargmNum = input("Enter a valid number: ")

			EargmIds = []
			for i in range(1, int(eargmNum)+1):
				EargmIds.append(i)

			EargmIds2 = []
			for i in range(1, int(eargmNum)+1):
				EargmIds2.append(i)

			metaEargmIdlst = []
			metaEargmIdslst = []
			for i in range(1, int(metaEargmNum)+1):
				print("Configuring meta-EARGM with ID {}: ".format(i))		

			metaEargmId = input("Which EARGM would be in charge of managing the distribution? [{}]: ".format("/".join(str(i) for i in EargmIds)))
			while metaEargmId == '' or not(metaEargmId.strip().isdigit()) or int(metaEargmId) not in EargmIds:
				metaEargmId = input("Enter a valid EARGM ID: ")

			EargmIds.remove(int(metaEargmId))
			metaEargmIdlst.append(metaEargmId)

			metaEargmIds = input("What EARGMs should it control (comma separated list)? [{}]: ".format(", ".join(str(i) for i in EargmIds2)))
			pattern = re.compile(r"^(\w+)(,\s*\w+)*$")
			metaEargmIdsList = metaEargmIds.split(",")
			while metaEargmIds == '' or pattern.match(metaEargmIds) == None or not all(int(x) in EargmIds2 for x in metaEargmIdsList):
				metaEargmIds = input("Enter a valid EARGM IDs: ")

			for i in metaEargmIdsList:
				EargmIds2.remove(int(i))

			metaEargmIdslst.append(metaEargmIdsList)
			print("")

		if metaEargm in ['n', 'N']:
			print("No meta-EARGM is configured")
		print("")

if powerCap in ['n', 'N']:
        print("No Power Capping is configured\n")
print("")

print("================================")
print("= ENERGY CAPPING Configuration =")
print("================================")
print("")

energyCap = input("Would you like to configure energy capping? [Y/n]: ")
if energyCap == '':
	energyCap = 'y'
while not(energyCap in ['y', 'Y', 'n', 'N']) or energyCap.strip().isdigit():
	if energyCap == '':
		energyCap = 'y'
		break
	energyCap = input("[Y/n]: ")

if energyCap in ['y', 'Y']:
	T1 = input("How often should the EARGM monitor for energy status (in seconds)? [600]: ")
	if T1 == '':
		T1 = 600
	while not(str(T1).strip().isdigit()):
		T1 = input("Enter a valid time (in seconds)? [600]: ")
		if T1 == '':
			T1 = 600

	T2 = input("What time period (in seconds) should the EARGM use to monitor the energy (for example, the last month)? [2592000]: ")
	if T2 == '':
		T2 = '2592000'
	while not(str(T2).strip().isdigit()):
		T2 = input("Enter a valid time (in seconds)? [2592000]: ")
		if T2 == '':
			T2 = '2592000'

	EARGMUnits = input("In which unit do you want to specify the energy? [J/K/M]: ")
	if EARGMUnits == 'j':
		EARGMUnits ='J'
	if EARGMUnits == 'k':
		EARGMUnits ='K'
	if EARGMUnits == 'm':
		EARGMUnits ='M'
	while EARGMUnits == '' or EARGMUnits.strip().isdigit() or not(EARGMUnits in ['j', 'J', 'k', 'K', 'm', 'M']):
		EARGMUnits = input("Enter a valid unit [J/K/M]: ")
		if EARGMUnits == 'j':
			EARGMUnits ='J'
		if EARGMUnits == 'k':
			EARGMUnits ='K'
		if EARGMUnits == 'm':
			EARGMUnits ='M'

	energy_total = 0
	for (i, j) in zip(nodesNumList, averagePowerList):
		energy = int(i) * int(j)
		energy_total += energy

	if EARGMUnits in ['j', 'J']:
		unit = 1
	if EARGMUnits in ['k', 'K']:
		unit = 1000
	if EARGMUnits in ['m', 'M']:
		unit = 1000000

	limit = (int(T2) * energy_total)/int(unit)

	energyLimit = input("What is the target energy limit? [{}]: ".format(int(limit)))
	if energyLimit == '':
		energyLimit = int(limit)
	while not(str(energyLimit).strip().isdigit()):
		energyLimit = input("Enter a valid number [{}]: ".format(int(limit)))
		if energyLimit == '':
			energyLimit = int(limit)

	EARGMUseAggregated = input("To do energy control, an EARGM will read data from the configured database.\nShould it use the aggregated data (better performance) instead of the per-node data? [Y/n]: ")
	if EARGMUseAggregated == '':
		EARGMUseAggregated = 'y'
	while not(EARGMUseAggregated in ['y', 'Y', 'n', 'N']) or EARGMUseAggregated.strip().isdigit():
		EARGMUseAggregated = input("[Y/n]: ")
		if EARGMUseAggregated == '':
			EARGMUseAggregated = 'y'
	if EARGMUseAggregated in ['y', 'Y']:
		EARGMUseAggregated = 1
	if EARGMUseAggregated in ['n', 'N']:
		EARGMUseAggregated = 0

	EARGMWarnings = input("Would you like to set up warnings when certain energy thresholds are passed? [Y/n]: ")
	if EARGMWarnings == '':
		EARGMWarnings = 'y'
	while not(EARGMWarnings in ['y', 'Y', 'n', 'N']) or EARGMWarnings.strip().isdigit():
		EARGMWarnings = input("[Y/n]: ")
		if EARGMWarnings == '':
			EARGMWarnings = 'y'

	if EARGMWarnings in ['y', 'Y']:
		EARGMWarningsPerc = input("Please introduce the thresholds separated by a comma [85,90,95]: ")
		if EARGMWarningsPerc == '':
			EARGMWarningsPerc = '85,90,95'
		pattern = re.compile(r"^(\w+)(,\s*\w+)*$")
		flag = 0
		i = 1
		numbers = [int(x.strip()) for x in EARGMWarningsPerc.split(',')]
		while i < len(numbers):
			if(numbers[i] < numbers[i - 1]):
				flag = 1
			i += 1
		while pattern.match(EARGMWarningsPerc) == None or (len([int(x.strip()) for x in EARGMWarningsPerc.split(',')]) != 3) or flag > 0:
			EARGMWarningsPerc = input("Enter a valid three values from lowest to highest with comma separated format [85,90,95]: ")
			if EARGMWarningsPerc == '':
				EARGMWarningsPerc = '85,90,95'

		Email = input("Do you want to receive an email when the thresholds are passed? [Y/n]: ")
		if Email == '':
			Email = 'y'
		while not(Email in ['y', 'Y', 'n', 'N']) or Email.strip().isdigit():
			Email = input("[Y/n]: ")
			if Email == '':
				Email = 'y'

		if Email in ['y', 'Y']:
			EARGMMail = input("Please introduce an email address: ")
			while EARGMMail == '' or EARGMMail.strip().isdigit():
				EARGMMail = input("Enter a valid email address: ")

		if Email in ['n', 'N']:
			EARGMMail = 'nomail'

		EARGMEnergyAction = input("Do you want to execute a command each time a threshold is reached? [Y/n]: ")
		if EARGMEnergyAction == '':
			EARGMEnergyAction = 'y'
		while not(EARGMEnergyAction in ['y', 'Y', 'n', 'N']) or Email.strip().isdigit():
			EARGMEnergyAction = input("[Y/n]:")
			if EARGMEnergyAction == '':
				EARGMEnergyAction = 'y'

		if EARGMEnergyAction in ['y', 'Y']:
			EARGMEnergyActionCommand = input("Please introduce the absolute path to the command (It will be executed with the following arguments\ncommand_name energy_T1 energy_T2  energy_limit T2 T1  units: ")
			while EARGMEnergyActionCommand == '' or EARGMEnergyActionCommand.strip().isdigit():
				EARGMEnergyActionCommand = input("Enter a valid command: ")
			print("")

		if EARGMEnergyAction in ['n', 'N']:
			EARGMEnergyActionCommand = "no_action"
		print("")

	if EARGMWarnings in ['n', 'N']:
		print("No energy thresholds configured")
		print("")

if energyCap in ['n', 'N']:
	print("No energy capping is configured")
	print("")

EARGMUseLog = input("Should the EARGM use a log file? [Y/n]: ")
if EARGMUseLog == '':
	EARGMUseLog = 'y'
while not(EARGMUseLog in ['y', 'Y', 'n', 'N']) or EARGMUseLog.strip().isdigit():
	EARGMUseLog = input("[Y/n]: ")
	if EARGMUseLog == '':
		EARGMUseLog = 'y'
if EARGMUseLog in ['y', 'Y']:
	EARGMUseLog = 1
if EARGMUseLog in ['n', 'N']:
	EARGMUseLog = 0
print("")

'''
#Tags
if EARGMPowerCapMode in ['0', '2']:
	for i in eargmTypeList:
		percentage = ((abs(int(eargmPowerList[i])) - eargmPowerSug)*100)/eargmPowerSug
		if int(i) == 0:
			i == 'all_types'
			print("Tag={} powercap=1 max_powercap={}".format(str(i), averagePowerList[i]*percentage))

if EARGMPowerCapMode in ['1']:
	for i in eargmTypeList:
		percentage = ((abs(int(eargmPowerList[i])) - eargmPowerSug)*100)/eargmPowerSug
		if int(i) == 0:
			i == 'all_types'
			print("Tag={} powercap={} max_powercap={}".format(str(i), averagePowerList[i]*percentage, maxPowerList[i]))

#Islands
'''








print("====================================================================================================")
print("====================================================================================================")
print("")
print("#---------------------------------------------------------------------------------------------------")
print("# EAR Global Manager (EARGMD): Update that section to use EARGM.")
print("#---------------------------------------------------------------------------------------------------")
print("#")
print("# Use aggregated periodic metrics or periodic power metrics.")
print("# Aggregated metrics are only available when EARDBD is running.")
print("#")
print("EARGMUseAggregated=", EARGMUseAggregated, sep='')
print("# Period T1 and T2 are specified in seconds (ex. T1 must be less than T2, ex. 10min and 1 month).")
print("EARGMPeriodT1=", T1, sep='')
print("EARGMPeriodT2=", T2, sep='')
print("# '-' are Joules, 'K' KiloJoules and 'M' MegaJoules.")
print("EARGMUnits=", EARGMUnits, sep='')
print("EARGMPort=50000")
print("# Two modes are supported '0=manual' and '1=automatic'.")
print("EARGMMode=1")
print("# Email address to report the warning level (and the action taken in automatic mode).")
print("EARGMMail=", EARGMMail, sep='')
print("# Percentage of accumulated energy to start the warning DEFCON level L4, L3 and L2.")
print("EARGMWarningsPerc=", EARGMWarningsPerc.replace(" ",""), sep='')
print("# T1 \"grace\" periods between DEFCON before re-evaluate.")
print("EARGMGracePeriods=3")
print("# Verbosity")
print("EARGMVerbose=0")
print("# When set to 1, the output is saved in 'TmpDir'/eargmd.log (common configuration) as a log file.")
print("EARGMUseLog=", EARGMUseLog, sep='')
print("#")
print("# Format for action is: command_name energy_T1 energy_T2  energy_limit T2 T1  units")
print("# This action is automatically executed at each warning level (only once per grace periods).")
print("#")
print("EARGMEnergyAction=", EARGMEnergyActionCommand, sep='')
print("# Period at which the powercap thread is activated. Meta-EARGM checks the EARGMs it controls every 2*EARGMPowerPeriod")
print("EARGMPowerPeriod=", EARGMPowerPeriod, sep='')
print("# Powercap mode: 0 is monitoring, 1 is hard powercap, 2 is soft powercap.")
print("EARGMPowerCapMode=", EARGMPowerCapMode, sep='')
print("# Admins can specify to automatically execute a command in EARGMPowerCapSuspendAction when total_power >= EARGMPowerLimit*EARGMPowerCapResumeLimit/100")
print("EARGMPowerCapSuspendLimit=", EARGMPowerCapSuspendLimit, sep='')
print("# Format for action is: command_name current_power current_limit total_idle_nodes total_idle_power")
print("EARGMPowerCapSuspendAction=", EARGMPowerCapSuspendAction, sep='')
print("#")
print("# Admins can specify to automatically execute a command in EARGMPowerCapResumeAction to undo EARGMPowerCapSuspendAction")
print("# when total_power >= EARGMPowerLimit*EARGMPowerCapResumeLimit/100.")
print("# Note that this will only be executed if a suspend action was executed previously.")
print("#")
print("EARGMPowerCapResumeLimit=", EARGMPowerCapResumeLimit, sep='')
print("# Format for action is: command_name current_power current_limit total_idle_nodes total_idle_power")
print("EARGMPowerCapResumeAction=", EARGMPowerCapResumeAction, sep='')
print("# EARGMs must be specified with a unique id, their node and the port that receives remote")
print("# connections. An EARGM can also act as meta-eargm if the meta field is filled, and it will")
print("# control the EARGMs whose ids are in said field. If two EARGMs are in the same node,")
print("# setting the EARGMID environment variable overrides the node field and chooses the characteristics")
print("# of the EARGM with the correspoding id.")
print("#")
print("# Only one EARGM can currently control the energy caps, so setting the rest to 0 is recommended.")
print("# energy = 0 -> energy_cap disabled")
print("# power = 0  -> powercap disabled")
print("# power = N  -> powercap budget for that EARGM (and the nodes it controls) is N")
print("# power = -1 -> powercap budget is calculated by adding up the powercap set to each of the nodes under its control.")

for i in range(len(eargmIdList)):
	if i == 0:
		if str(eargmIdList[i]) in metaEargmIdlst:
			print("EARGMId={} energy={} power={} node={} port={} meta={}".format(eargmIdList[i], energyLimit, eargmPowerList[i], eargmHostnameList[i], eargmPortList[i], ','.join(metaEargmIdslst[metaEargmIdlst.index(str(eargmIdList[i]))])))
		else:
			print("EARGMId={} energy={} power={} node={} port={}".format(eargmIdList[i], energyLimit, eargmPowerList[i], eargmHostnameList[i], eargmPortList[i]))

	else:
		if str(eargmIdList[i]) in metaEargmIdlst:
			print("EARGMId={} energy=0 power={} node={} port={} meta={}".format(eargmIdList[i], eargmPowerList[i], eargmHostnameList[i], eargmPortList[i], ','.join(metaEargmIdslst[metaEargmIdlst.index(str(eargmIdList[i]))])))
		else:
			print("EARGMId={} energy=0 power={} node={} port={}".format(eargmIdList[i], eargmPowerList[i], eargmHostnameList[i], eargmPortList[i]))
