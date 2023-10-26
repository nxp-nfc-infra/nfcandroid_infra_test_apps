/******************************************************************************
 * Copyright 2022 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
  char insmod_str[56] =
      "insmod /vendor/lib/modules/nxpnfc_i2c.ko i2c_sw_param=";
  if (argc == 3 && strlen(argv[2]) == 1) {
    strcat(insmod_str, argv[2]);
    system("service call nfc 7");
    if (strcmp(argv[2], "1") == 0) {
      puts("Short Circuit j55 connector and press any key to continue");
    } else if (strcmp(argv[2], "0") == 0) {
      puts("Remove j55 jumper and press any key to continue");
    }
    getchar();
    system("rmmod nxpnfc_i2c.ko");
    system(insmod_str);
    system("service call nfc 8");
    return 0;
  } else {
    puts("Wrong arguments");
    puts(
        "\t\tI2C interface driver utility\n\n --h \t\t help\n -i\t\t Selects "
        "I2C-M or I2C-HIF\n\t\t 0-->I2C-HIF\n\t\t 1-->I2C-M");
    return 1;
  }
}