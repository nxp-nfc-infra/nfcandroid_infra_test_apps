###################################################################################################
#                             DUAL CPU Mode Switch test tool                                      #
###################################################################################################
•  Folder Name: SMCU_Switch
•  Contents:
   o    SmcuSwitchV2_0TargetTest.cpp      -> Source file
   o    Android.bp                        -> Make File
   o    README.txt                        -> How to build and use executable.

•  How to build the executable:
   o STEP1: Keep SMCU_Switch folder under packages/apps/
   o STEP2: Navigate to the SMCU_Switch folder and run mm -j32 command.
   o STEP3: It will build SmcuSwitchV2_0 under following folder
             out/target/product/db845c/testcases/SmcuSwitchV2_0/arm64/

•  How to do SMCU_Switch:
   o STEP1: Push this file to system/lib64/ folder
             i.e. adb push SmcuSwitchV2_0 /system/bin/
   o STEP2: Run SmcuSwitchV2_0 from command line.

   o STEP3: It will ask for USER input. Enter valid input
              Select the option
                 1. Switch to EMVCo Mode (NFCC will switch to SMCU interface)
                 2. Switch to NFC Mode   (NFCC will switch to NFC interface)
                 3. Switch to EMVCo FW DNLD Mode (NFCC will switch to SMCU interface)
                 4. Exit execution       (Application will stop the execution)

•  Note:
   o If you want colored o/p to be printed, please use --gtes_color=yes flag.
   o If you want o/p to be stored in xml file, please use --gtes_output=[xml:"FILE_PATH"] flag.
        e.g.
           $SmcuSwitchV2_0 --gtes_color=yes --gtes_output=xml:"system/bin/SmcuSwitch.xml"
###################################################################################################
