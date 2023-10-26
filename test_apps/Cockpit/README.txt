###################################################################################################
#     Cockpit Utility:                                                                            #
#             This utilite will dump the EEPROM and protocol data into NFCC                       #
###################################################################################################
•  Folder Name: Cockpit
•  Contents:
   o    main.cpp                             -> Source file
   o    cockpit.cpp                          -> Source file
   o    pugixml.cpp                          -> Lib file
   o    Android.bp                           -> Make File
   o    README.txt                           -> How to build and use execute.

•  How to build the source code:
   o STEP1: Keep Cockpit folder under android/packages/apps/
   o STEP2: Navigate to the Cockpit folder and run mm -j32 command to build the src code.
   o STEP3: It will create CockPit_Util_V1_0 executable file under following folder
            android/out/target/product/db845c/testcases/CockPit_Util_V1_0/arm64

•  How to run the Cockpit Utility:
   o STEP1: Push this file to /data/nativetest64/ folder
             i.e.
                adb root
                adb remount
                adb push CockPit_Util_V1_0 /data/nativetest64/
   o STEP2: Push the Pn72xx_DumpData.xml file to /vendor/etc/
             i.e.
                adb root
                adb remount
                adb push Pn72xx_DumpData.xml /vendor/etc/
            Note : If the file name is different then rename the file while pushing it. by doing this
                adb push PN5190_DumpEEPROM.xml /vendor/etc/PN72xx_DumpData.xml

   o STEP3: Run CockPit_Util_V1_0 from command line.
             i.e
                adb shell
                cd /data/nativetest64/
                chmod 777 CockPit_Util_V1_0
                ./CockPit_Util_V1_0

             Executable will print below information On console
                1. Which test case is currently Running.
                2. Is it passed or failed with time taken to execute.
                3. At last, it will print the summary with no of passed and
                failed test cases(with name of failed tests).

•  Note:
   o If you want colored o/p to be printed, please use --gtes_color=yes flag.
   o If you want o/p to be stored in xml file, please use --gtes_output=[xml:"FILE_PATH"] flag.
        e.g.
           $CockPit_Util_V1_0 --gtes_color=yes --gtes_output=xml:"system/bin/Cockpit.xml"
##########################################################################################################
