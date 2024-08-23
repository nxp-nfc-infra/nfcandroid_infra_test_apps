###################################################################################################
#     NFC SelfTest:                                                                               #
#                   PRBS, ANTENNA                               No.of Tests=22               #
###################################################################################################
•  Folder Name: SelfTeset
•  Contents:
   o    SelfTestHalAidlNfcTargetTest.cpp  -> Source file
   o    Android.bp                        -> Make File
   o    README.txt                        -> How to build and use executable.

•  How to build the executable:
   o STEP1: Keep SelfTest folder under packages/apps/
   o STEP2: Navigate to the SelfTest folder and run mma command.
   o STEP3: It will build SelfTestHalAidlNfc.exe under following folder
             out/target/product/db845c/data/nativetest/SelfTestHalAidlNfc/arm64/

•  How to do SelfTest:
   o STEP1: Push this file to system/bin/ folder
             i.e. adb push SelfTestHalAidlNfc /system/bin/SelfTestHalAidlNfc
   o STEP2: Run SelfTestHalAidlNfc.exe from command line.
             Executable will print below information On console
                1. Which test case is currently Running.
                2. Is it passed or failed with time taken to execute.
                3. At last, it will print the summary with no of passed and
                failed test cases(with name of failed tests).

•  Note:
   o If you want colored o/p to be printed, please use --gtes_color=yes flag.
   o If you want o/p to be stored in xml file, please use --gtes_output=[xml:"FILE_PATH"] flag.
        e.g.
           $SelfTestHalAidlNfc --gtes_color=yes --gtes_output=xml:"system/bin/SelfTest.xml"
###################################################################################################