/******************************************************************************
 * Copyright 2018 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
******************************************************************************
* CHANGES AUTHOR   : Gustavo Costa Gomes de Melo (gustavocosta@ic.ufal.br)
* CHANGES HISTORY  :
*   01/11/2021 - Added project data, Wi-Fi information and certificate.
*   01/13/2021 - Changing defalt NTP servers tp NTP.BR
*   03/22/2021 - Removing project data, Wi-Fi information and certificate for publication.
*****************************************************************************/
// This file contains your configuration used to connect to Cloud IoT Core

// Wifi network details.
const char *ssid = "SSID";
const char *password = "PASSWORD";

// Cloud iot details.
const char *project_id = "project-id";
const char *location = "us-central1";
const char *registry_id = "my-registry";
const char *device_id = "my-esp32-device";

// Configuration for NTP
const char* ntp_primary = "a.ntp.br";
const char* ntp_secondary = "b.ntp.br";

#ifndef LED_BUILTIN
#define LED_BUILTIN 25
#endif

// To get the private key run (where private-key.pem is the ec private key
// used to create the certificate uploaded to google cloud iot):
// openssl ec -in <private-key.pem> -noout -text
// and copy priv: part.
// The key length should be exactly the same as the key length bellow (32 pairs
// of hex digits). If it's bigger and it starts with "00:" delete the "00:". If
// it's smaller add "00:" to the start. If it's too big or too small something
// is probably wrong with your key.
const char *private_key_str = "private_key_str";

// Time (seconds) to expire token += 20 minutes for drift
const int jwt_exp_secs = 3600*24; // Maximum 24H (3600*24)

// To get the certificate for your region run:
//   openssl s_client -showcerts -connect mqtt.googleapis.com:8883
// for standard mqtt or for LTS:
//   openssl s_client -showcerts -connect mqtt.2030.ltsapis.goog:8883
// Copy the certificate (all lines between and including ---BEGIN CERTIFICATE---
// and --END CERTIFICATE--) to root.cert and put here on the root_cert variable.

const char *root_cert =
"certificate";

// In case we ever need extra topics
//const int ex_num_topics = 0;
//const char* ex_topics[ex_num_topics];
//const int ex_num_topics = 1;
//const char* ex_topics[ex_num_topics] = {
//  "/devices/my-device/tbd/#"
//};
