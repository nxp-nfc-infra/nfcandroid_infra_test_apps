###################################################################################################
#     NFC SlefTest:                                                                               #
#             RF Field Switch, CTS and Test Signal configuration     No.of Tests=5                #
###################################################################################################
•  Folder Name: SelfTeset
•  Contents:
   o    main.cpp                             -> Source file
   o    NxpNfc_CTS.cpp                       -> Source file
   o    NxpNfc_PRBS.cpp                      -> Source file
   o    NxpNfc_RfSwitch.cpp                  -> Source file
   o    NxpNfc_EEPROMConfig.cpp              -> Source file
   o    Android.bp                           -> Make File
   o    README.txt                           -> How to build and use execute.

•  How to build the executable:
   o STEP1: Keep SelfTest folder under android/packages/apps/
   o STEP2: Navigate to the SelfTest folder and run mm -j32 command to build the src code.
   o STEP3: It will create SelfTest_Util_V1_0.exe under following folder
            out/target/product/db845c/testcases/SelfTest_Util_V1.0/arm64/

•  How to do SelfTest:
   o STEP1: Push this file to /data/nativetest64/ folder
             i.e.
                adb root
                adb remount
                adb push SelfTest_Util_V1_0 /data/nativetest64/
   o STEP2: Run SelfTest_Util_V1_0.exe from command line.
             i.e
                adb shell
                cd /data/nativetest64/
                chmod 777 SelfTest_Util_V1_0
                ./SelfTest_Util_V1_0

             Executable will print below information On console
                1. Which test case is currently Running.
                2. Is it passed or failed with time taken to execute.
                3. At last, it will print the summary with no of passed and
                failed test cases(with name of failed tests).

•  Note:
   o If you want colored o/p to be printed, please use --gtes_color=yes flag.
   o If you want o/p to be stored in xml file, please use --gtes_output=[xml:"FILE_PATH"] flag.
        e.g.
           $SelfTest_Util_V1_0 --gtes_color=yes --gtes_output=xml:"system/bin/SelfTest.xml"
##########################################################################################################
