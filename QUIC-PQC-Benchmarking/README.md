# Post‑Quantum QUIC Demo with **OpenSSL 3.5.0** & **OQS‑Provider**

This repository demonstrates how to build OpenSSL 3.5.0 with QUIC support, add the [Open Quantum Safe](https://openquantumsafe.org) *oqs‑provider* for post‑quantum algorithms, and run the bundled QUIC demo using PQC algorithms.

---
## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Build and Install OpenSSL 3.5.0](#build-and-install-openssl-350)
3. [Build the OQS-Provider](#build-the-oqsprovider)
4. [Configure OpenSSL to load the OQS-Provider](#configure-openssl-to-load-the-oqs-provider)
5. [Usage](#usage)
6. [Measure handshake RTT with script for benchmarking](#measure-handshake-rtt-with-script-for-benchmarking)

---
## Prerequisites
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git
```

---
## Build and Install OpenSSL 3.5.0
1. Enter the root folder OpenSSL 3.5.0 after cloning this repository:
   ```bash
   cd openssl-3.5.0
   ```
2. Configure and build OpenSSL 3.5.0:
   ```bash
   ./Configure
   make
   ```
3. Build the QUIC demo server:
   ```bash
   cd demos/quic/server
   make
   ```
4. Expose the built libraries:
   ```bash
   echo 'export LD_LIBRARY_PATH=$HOME/opt/openssl-3.5.0/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
   source ~/.bashrc
   ```

---
## Build the OQS‑Provider
```bash
git clone https://github.com/open-quantum-safe/oqs-provider.git
cd oqs-provider
OPENSSL_INSTALL=<ROOT_DIR_TO_OPENSSL-3.5.0> ./scripts/fullbuild.sh
```
The provider shared object is produced at:
```
_build/lib/oqsprovider.so
```
Note the absolute path, you will reference it in the **Configure** step below.

---
## Configure OpenSSL to load the OQS-Provider
1. Locate the active *openssl.cnf* used by your new OpenSSL build:
   ```bash
   <ROOT_DIR_TO_OPENSSL-3.5.0>/apps/openssl version -d
   # e.g. OPENSSLDIR: "/usr/local/ssl"
   ```
2. Edit `<OPENSSLDIR>/openssl.cnf` and make the following changes:

   *Under `[provider_sect]`* add:
   ```ini
   oqsprovider = oqsprovider_sect
   ```
   
   *Under `[default_sect]`* **comment‑out** or **add** the automatic activation:
   ```ini
   #activate = 1
   ```
   
   After `[default_sect]` append the new provider section:
   ```ini
   [oqsprovider_sect]
   activate = 1
   module = <ABSOLUTE_PATH_TO_OQS-PROVIDER>/_build/lib/oqsprovider.so
   ```
3. Save the file.

---
## Usage
### 1. Go into the root directory of OpenSSL 3.5.0
### 2. Generate a certificate for DSAs
```bash
# 1. Generate a private key (ML‑DSA‑44)
./apps/openssl genpkey -algorithm MLDSA44 -out mldsa44.key

# 2. Create a self‑signed certificate
./apps/openssl req -new -x509 \
    -key mldsa44.key -out mldsa44.crt \
    -days 365 -subj "/CN=localhost"
```
In this example **ML-DSA-44** has been used as the algorithm but other DSAs can be used instead. Run the command `./apps/openssl list -signature-algorithms` to list all possible DSAs to use.

### 3. Run the QUIC server
```bash
./demos/quic/server/server 4433 mldsa44.crt mldsa44.key
```

### 4. Run the QUIC client
```bash
./apps/openssl s_client \
    -quic -alpn ossltest -connect <ADDRESS>:<PORT>
```
Additionaly the option `-handshaketime` can be added to print out the Handshake-RTT duration. This is only available in this OpenSSL implementation inside this repository.

### 5. Change key‑exchange algorithms (optional)
In `demos/quic/server/server.c` locate the optional call to `SSL_CTX_set1_groups_list` and edit it to suit your needs, e.g. to prefer only ML‑KEM:
```c
/* Optionally set key exchange. */
/*
if (!SSL_CTX_set1_groups_list(ctx, "x25519:MLKEM512:MLKEM768:MLKEM1024")) {
    fprintf(stderr, "failed to set key exchange group\n");
    goto err;
}
*/
```
Replace the group list string, then rebuild the server:
```bash
cd demos/quic/server && make
```

---
## Measure handshake RTT with script for benchmarking

The helper script **`measure-handshake.sh`** lets you run multiple connections for benchmarking handshake latency.

```text
usage: ./measure-handshake.sh
```
Inside the script, the following variables need to be set:
| Variable       | Description                                                                                 |
| -------------- | ------------------------------------------------------------------------------------------- |
| **OUTFILE**    | File name where RTT values (ms) are being written                                           |
| **OPENSSL_BIN**| Absolute path to the *openssl* binary you built in the previous steps                        |
| **SERVER**     | Target server in `host:port` format, e.g. `localhost:4433`                                   |
| **COUNT**      | Number of handshake iterations to perform                                                   |
| **KEY_EXCHANGE** | *(optional)* Colon‑separated group list passed via `-groups`, e.g. `x25519:MLKEM512`        |

### Example: 1 000 ML‑KEM‑512 handshakes
```bash
OUTFILE=handshake_rtt.txt
OPENSSL_BIN=$HOME/openssl-3.5.0/apps/openssl
SERVER=localhost:4433
COUNT=1000
KEY_EXCHANGE=MLKEM512
```
After the script finishes you’ll have `handshake_rtt.txt` with 1 000 RTT values (ms).
