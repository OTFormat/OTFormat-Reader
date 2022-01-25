# OTFormat Reader

OTFomat Reader, is a software provided openly and freely as binary or open source, which provides long term access to object data and metadata from a magnetic tape cartridge formatted with OTFormat. 

Click [here](https://asset.fujifilm.com/www/jp/files/2021-11/c1b8c0af464bb9135fc3f68fa20b56af/OTFormat-Reader_1.02_BINARY.zip) if you are interested in downloading the binary of OTFormat Reader.

## OTFormat Specifications

OTFomat is the optimized format to manage object data and metadata in magnetic tape cartridges.

OTFormat consists of two partitions, one is the Reference Partition and the other is the Data Partition. 

By having metadata in the Reference Partition, accessibility and searchability are enhanced.

Metadata is also written to the Data Partition to enhance redundancy, while maintaining the usable data capacity of magnetic tape cartridges. 

Also, OTFormat is a self-describing format which enables data and its meta data to be directly retrieved from a single tape cartridge directly. Details please click [here](https://asset.fujifilm.com/www/jp/files/2021-04/9518fd348ccb9108440825bef2af56cb/OTFormat_Specification_ver2.0.0.pdf) if you are interested in accessing the OTFormat specification.  

## Requirements

	Operating System: 	Red Hat Enterprise Linux 7.9 & 8.4, CentOS 7.9
	CPU: 			x86_64 architecture
	Memory: 		>64MB
	Disk size: 		(MINIMUM)        > 100GB
				(RECOMMENDATION) > 100GB + a total capacity of objects you want to read from a tape
	Tape drive: 		LTO7, LTO8, LTO9, TS1155 or TS1160 tape drive
	Tape: 			LTO7, LTO8, LTO9, JD, JE, JL or JM tape formatted with the OTFormat
	Compiler (OPTIONAL): 	gcc, version 4.8.5 20150623, was used for making the binary

## How to use

Follow either 1 or 2, and then load a tape to a drive and run this tool at your environment which satisfies the Requirements above.
1. Download a binary file from [here](https://asset.fujifilm.com/www/jp/files/2021-11/c1b8c0af464bb9135fc3f68fa20b56af/OTFormat-Reader_1.02_BINARY.zip) and deploy to your machine.
2. Download the source codes and build them. Please refer to the following section “How to build”.

NOTE:
- You should read tapes one by one.
- You shall not use a tape drive with other software when OTFormat Reader is working with the tape drive. 

### Typical usage

OTFormat Reader has a command line interface with the following structure

	./sdt-otformat-reader -d <device_name> [options and parameters]

where, -d <device_name> is a required option to specify a tape drive with a tape formatted with the OTFormat. 
All of the options and parameters are shown in the following section “Options”. 

5 types of usage are introduced here. 
1. Read the latest version of an object in a bucket.

		./sdt-otformat-reader -d /dev/sg4 -b your-bucket-name -o your-object-key -s /mnt/save_path/
		
2. Make a list of all objects per bucket in a tape which includes an object metadata such as KEY, ID, SIZE, LAST-MODIFIED-DATE, etc.

		./sdt-otformat-reader -d /dev/sg4 -l -s /mnt/save_path/ 

3. Read a versioned object in a bucket.

		./sdt-otformat-reader -d /dev/sg4 -O d41d8cd98f00b204e9800998ecf8427e -b your-bucket-name -o your-object-key -s /mnt/save_path/
	where, d41d8cd98f00b204e9800998ecf8427e is a versioned object ID which is able to be acquired from a list above.  

4. Read all objects from a tape formatted with the OTFormat. 

		./sdt-otformat-reader -d /dev/sg4 -f -s /mnt/save_path/

5. Resume to read all objects from a point when interrupted during Full dump. 

		./sdt-otformat-reader -d /dev/sg4 -r -s /mnt/save_path/ 
	NOTE: if you want, you can change the tape drive.

NOTE: You may need to change “/dev/sg4” and “/mnt/save_path” to appropriate values under your environment.
      “lsscsi -g” command may be convenient to acquire a device name.


### Options
	-b, --bucket          = <name>   Specify a bucket name in which an object you specified is stored.
	-d, --drive           = <name>   Specify a device name of a tape drive.
	-F, --Force           : Avoid to check a disk space during either Full or Resume dump.
	-f, --full-dump       : Read all objects from a tape formatted with the OTFormat.
	-h, --help
	-i, --interval        : Output a progress to "history.log" during either Full dump or Resume dump.
	-L, --Level           = <value>  Specify an output level. default is 0
					 0: Object Data and Meta
					 1: Packed Object
	-l, --list            : Output a list of all objects in each bucket stored in a tape.
	-o, --object-key      = <name>   Specify an object KEY.
	-O, --Object-id       = <ID or Option> Specify an Object version. default is "latest".
				<ID>     Specify a versioned object ID, which will be shown in a list file.
				<Option> Either "latest" or "all" is available.
					 "latest" : Output ONLY the latest version object. 
					 "all"    : Output ALL versions with the Object-Key. 
	-r, --resume-dump     : Resume a Full dump process when "history.log" file was updated.
	-s, --save-path       = <path>   Specify a full path where data will be stored. 
					 Default is the application path.
	-v, --verbose         = <level>  Specify output_level.
					 If this option is not set, no progress will be displayed.
					 v:information about header.
					 vv:information about L4 in addition to above.
					 vvv:information about L3 in addition to above.
					 vvvv:information about L2 in addition to above.
					 vvvvv:information about L1 in addition to above.
					 vvvvvv:information about MISC for MAM and others in addition to above.


### Output directory structure

	sdt-otformat-reader
	history.log                             History information during either Full or Resume dump.
	reference_partition                     Temporary data stored in the Reference partition of a tape.
	  ├── OTFLabel
	  ├── PR_0
	  ├── PR_1
	  ├── PR_2
	  ├── ...
	  ├── RCM_0
	  ├── RCM_1
	  └── VOL1Label
	  
	<save_path>                             Same name as you specified -s option parameter.
	├── <tape_id>                           8 digits barcode of a tape.
	│   ├── reference_partition             Same data as reference_partition above. 
	│   │    ├── OTFLabel			
	│   │    ├── PR_0
	│   │    ├── PR_1
	│   │    ├── PR_2
	│   │    ├── ...
	│   │    ├── RCM_0
	│   │    ├── RCM_1
	│   │    └── VOL1Label
	│   ├── <bucketname>_0001.lst            List file, where <bucketname> is a name you specified as -b option, 
	│   ├── <bucketname>_0002.lst		 0001 shows a number of list file, and each file has 1000 objects data.
	│   ├── ...				 i.e. 1 million (=1000 list files x 1000 objects) is the largest number.
	│
	├── <packed_object_id>.pac               Packed object including an object in a bucket you specified.
	└── <bucketname>			
	    ├── 0000				 0000 is a special directory, which will be used for 
	    │   └── 0000                         reading the latest object or versioned object(s).
	    │       ├── object_key               Directories will be created automatically in case an object KEY includes "/".
	    │       │      ├──object_id.data     NOTE: "/" will be ignored if the last letter is "/".
	    │       │      └──  object_id.meta
	    │       ├── object_key
	    │       ├── ...
	    │    
	    ├── 0001                             0001 ~ 1000 directories will be created during Full or Resume dump. 
	    │   ├── 0001                         Each parent directory can store up to 1 million objects, 
	    │   │   ├── object_key		 and each subdirectory can store up to 1000 objects.
	    │   │   │      ├── object_id.data	 i.e. 1 billion (= 1000 parent directories x 1 million objects)  
	    │   │   │      └── object_id.meta	      is the largest number.
	    │   │   ├── object_key
	    │   │   ├── ...
	    │   │
	    │   ├── 0002
	    │   ├── ...
	    │   └── 1000
	    │
	    ├── 0002
	    ├── ...
	    └── 1000

### Restriction

- Deleted objects are readable from a tape even if delete markers are written in the tape.
- When a cartridge memory is not accessible, OTFormat Reader identifies a tape with the first six digit of a barcode label.

## How to build

### Prerequisites

1. [Eclipse IDE for C/C++ Developers](https://www.eclipse.org/downloads/download.php?file=/technology/epp/downloads/release/2021-03/R/eclipse-cpp-2021-03-R-linux-gtk-x86_64.tar.gz) has been installed.  
2. Check if the following libraries have been installed.  
```
    $ sudo yum list installed | grep -e json-c -e libuuid -e openssl 
        json-c.x86_64  
        json-c-devel.x86_64  
        libuuid.x86_64  
        libuuid-devel.x86_64
        openssl.x86_64
        openssl-libs.x86_64
```
3. Install them if they have not.  
```
    $ sudo yum install -y json-c  
    $ sudo yum install -y json-c-devel
    $ sudo yum install -y libuuid  
    $ sudo yum install -y libuuid-devel
    $ sudo yum install -y openssl  
    $ sudo yum install -y openssl-libs    
```

### Setup on Eclipse

1. Get project files.  
    Go to https://github.com/OTFormat/OTFormat-Reader
    Click "Clone or download" and get all files.  
    Make "otformat_reader" directory and put all files.  

```
	otformat_reader
	├── include
	│    └── ***.h
	├── src
	│    └── ***.c
	├── .cproject
	└── .project
```

2. Import projects.  
    Launch the eclipse.  
    Click "Import" in the File menu.    
    Click "Existing project into workspace" in "General" and click "Next".  
    Check "Select root directory" and click "Browse" on the right side.  
    Select the "otformat_reader" directory which you made in the 1st step, and click "OK".  
    Click "Finish".  
    The imported project will be added and displayed in the "Project Explorer".  

### Build the project

Right click on the "otformat_reader" in the "Project Explorer".  
Select "Build Configurations" then, "Build All".  
Executable file(sdt-otformat-reader) will be made.  

```
	otformat_reader  
	├── Release  
	│    └── sdt-otformat-reader  
	└── Debug  
	     └── sdt-otformat-reader  
```

## Changes
### Version 1.0.1
  - Fix minor issues.
### Version 1.0.2
  - Fix memory related bugs.

## License

Copyright 2021 FUJIFILM Corporation  

Licensed under the Apache License, Version 2.0 (the "License");  
you may not use this file except in compliance with the License.  
You may obtain a copy of the License at  

    http://www.apache.org/licenses/LICENSE-2.0  

Unless required by applicable law or agreed to in writing, software  
distributed under the License is distributed on an "AS IS" BASIS,  
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
See the License for the specific language governing permissions and  
limitations under the License.  
