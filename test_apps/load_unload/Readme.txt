###########################################################################
#                   I2C interface driver utility                          #
#                                                                         #
###########################################################################

Utility options available :
--h --> help
-i  --> Selects I2C-M or I2C-hif interface
        0 --> I2C-hif
        1 --> I2C-M
Example :- load_unload -i 1 --> Enable I2C-M as an interface

How to build executable binary
 o STEP1: Keep LOAD_UNLOAD folder under android/packages/apps/
 o STEP2: Navigate to the LOAD_UNLOAD folder and run mm -j16 command to build the src code.
 o STEP3: It will create "load_unload" binary under following folder
          out/target/product/db845c/system/bin/

How to switch between 2 interfaces
 
 Switch to I2C-m
 1) run the command "load_unload -i 1"
 2) connect the j55 jumper and press any key to continue.
 
 Switch to I2C-hif
 1) run the command "load_unload -i 0"
 2) remove the j55 jumper and press any key to continue.

 Note :- 
 1) I2C-hif is the default i2c interface during boot time.
 2) Remove the j55 jumper before booting the device
 3) default i2c interface can be changed through init.qcom.rc file
 4) device mode should be superuser while executing load_unload binary
 