# sbac-sgxdemo

SGX (_software guard extensions_) demonstrator presented during a [Tutorial](https://graal.ens-lyon.fr/sbac-pad/index.php/call-for-tutorials) in [SBAC-PAD 2018](https://graal.ens-lyon.fr/sbac-pad/). Makefiles and sources were based on sample code from the Intel SGX SDK.

## First, install:

- [Intel SGX SDK and PSW](https://github.com/intel/linux-sgx)
- [Intel SGX Driver](https://github.com/intel/linux-sgx-driver)

## Virtual address translation

To run SGX processes in hardware mode, one has to setup the PRM (_processor reserved memory_) in the BIOS (_basic input output system_). This reserved memory will contain the EPC (_enclave page cache_). In order to prevent malicious or inadvertent access, whenever the processor is not executing enclave code, the TLB (_translation look-aside buffer_) entries for virtual addresses that belong to the PRM are replaced with an abort page.

For more details, see:
- Full explanation [here](https://eprint.iacr.org/2016/086.pdf). 
- Resumed [here](https://insujang.github.io/2017-04-03/intel-sgx-protection-mechanism/).

This experiment shows how to perceive that behavior.

1. First, compile and run the `negative` example.
```
$ cd negative && make
<stuff>
The project has been built in debug hardware mode.
$ ./app
Pid: 9801
Enclave says: I do not like negative numbers
Result: -1
Enter a character before exit ...
```

2. With the process still running, find a virtual address that does not belong to the EPC:
```
$ cat /proc/9801/smaps | head -1
55a414757000-55a414759000 r-xp 00000000 08:02 13109653 /home/<username>/sbac-sgxdemo/negative/app
```

3. Translate that virtual address into physical. To do that, we used [this](http://fivelinesofcode.blogspot.com/2014/03/how-to-translate-virtual-to-physical.html) code, which corresponds to the `translate` executable bellow.

```
$ ~/translate 9801 55a414757000
Big endian? 0
Vaddr: 0x55a414757000, Page_size: 4096, Entry_size: 8
Reading /proc/9801/pagemap at 0x2ad20a3ab8
[0]0x0 [1]0x0 [2]0x0 [3]0x0 [4]0x0 [5]0x0 [6]0x80 [7]0xa1 
Result: 0xa180000000000000
PFN: 0x0
```

The virtual address `55a414757000` translated to physical address `0xa180000000000000`.

4. Now, find a virtual address that does belong to the EPC.
```
$ cat /proc/9801/smaps | grep isgx | tail -1
7f2f5b7ad000-7f2f5c000000 ---s 007ad000 00:06 421 /dev/isgx
```

5. And _try to_ translate it into physical:
```
$ ~/translate 9801 7f2f5b7ad000
Big endian? 0
Vaddr: 0x7f2f5b7ad000, Page_size: 4096, Entry_size: 8
Reading /proc/9801/pagemap at 0x3f97adbd68
[0]0x0 [1]0x0 [2]0x0 [3]0x0 [4]0x0 [5]0x0 [6]0x80 [7]0x0 
Result: 0x80000000000000
Page not present
```

The physical address corresponding to the virtual `7f2f5b7ad000` is not present in the translation table.

## Memory dump

Now, we will try to inspect the enclave memory. If you look in the source `negative/Enclave/Enclave.cpp`, you will find:

```
int ecall_compute(int a, int b) {
    const char *hidden = "sbac-pad";
    (void) hidden;
    int res = a + b;
    if(res < 0) {
        printf("I do not like negative numbers\n");
    }
    return res;
}
```

We are going to look for the string `sbac-pad` in the memory dump.

1. Dump the memory of the running process.
```
$ sudo gcore 9801
<stuff>
Saved corefile core.9801
```

2. Extract the strings and _try to_ filter `sbac-pad`.
```
$ strings core.9801 | grep sbac-pad
$
```

3. Nothing! So, let's try now to do the same in _simulation mode_, meaning that it only pretends (by using some special libraries) to run a SGX process in hardware mode. So, clean, compile and run.
```
$ kill 9801
$ make clean all SGX_MODE=SIM
<stuff>
The project has been built in debug simulation mode.
$ source /opt/intel/sgxsdk/environment
$ ./app
Pid: 9969
Enclave says: I do not like negative numbers
Result: -1
Enter a character before exit ...
```

4. Dump the memory and filter the string.  
```
$ sudo gcore 9969
<stuff>
Saved corefile core.9969
$ strings core.9969 | grep sbac-pad
sbac-pad
$ 
```

Voilà `sbac-pad`.

## Comparing sealings

There are two ways of sealing data: based on the enclave or the signer. Both of them include in the key derivation process the built-in key that is burned in the CPU, thus not allowing the data to be recovered in any other machine. The enclave-based sealing allows data decryption only by the exact same enclave, whereas the signer-based one grants plain-text access to any enclave that was signed with the same private-key.

This experiment shows that.

1. Compile and run the sealing demo.
```
$ cd sealing && make
$ ./app sbac-pad
``` 

The program works as follows: when a parameter is given, it seals `argv[1]` along with some random number between 0 and 99 generated inside the enclave. The result is put in the file `sealed.dat`. If no parameter is given, it tries to unseal the same file.

2. Now we should have a file with the sealed data in enlcave mode. Check it:
```
$ ./app
Enclave says: 'sbac-pad 12'
```

3. If you clean and compile the same code again, you are still able to retrieve the data.
```
$ make clean all && ./app
Enclave says: 'sbac-pad 12'
```

4. By changing only the private key, we can still retrieve the data.
```
$ openssl genrsa -out Enclave/private_key.pem -3 3072
$ make clean all && ./app
Enclave says: 'sbac-pad 12'
```

5. If you change anything in the enclave, on the other hand, you will not be able to unseal the data anymore.
```
$ sed -i 's/\<MAC\>/mac/g' Enclave/Enclave.cpp
$ make clean all && ./app
$ Enclave says: mac mismatch!
```

6. We will try now the signer-based sealing. Edit the file `sealing/Enclave/Enclave.cpp`. Make sure the two lines bellow look like this:
```
    //std::string sealed = sealEnclave( secret );
    std::string sealed = sealSigner( secret );
```

7. Compile, run, modify something in the enclave source, run again.
```
$ make clean all && ./app sgx-tutorial-sbac
$ ./app 
Enclave says: 'sgx-tutorial-sbac 76'
$ sed -i 's/\<mismatch/mIsMaTcH/g' Enclave/Enclave.cpp
$ make clean all && ./app
$ Enclave says: 'sgx-tutorial-sbac 76'
```

Since the same key was used to sign the enclave code, it is still possible to unseal the data in signer-based mode.

8. Change the private key used to sign the enclave, compile and run again.
```
$ openssl genrsa -out Enclave/private_key.pem -3 3072
$ make clean all && ./app
Enclave says: mac mIsMaTcH!
```

Since the key has changed, it is not possible to retrieve the content anymore.

