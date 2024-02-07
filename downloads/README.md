# Download examples dependencies
Examples require model files and some associated metadata (labels, ssd priors definitions...) to be downloaded so that they are made available locally to examples in top level `downloads` directory.<br>
This download procedure is to be done once from the host PC. A Jupyter notebook [download.ipynb](./download.ipynb) is available to fetch all dependencies from network and to apply necessary format-conversions.<br>
This Notebook execution requires:
1. eIQ Toolkit local install
2. Jupyter Notebook python package

# eIQ Toolkit install and environment setup
Last version of eIQ Toolkit installer can be downloaded from [NXP eIQ website](https://www.nxp.com/eiq).
It is available for Windows and Ubuntu OS platforms.<br>
For more information, refer to eIQ Toolkit User Guide available on [the documentation section of NXP eIQ website](https://www.nxp.com/design/software/development-software/eiq-ml-development-environment/eiq-toolkit-for-end-to-end-model-development-and-deployment:EIQ-TOOLKIT#documentation), and also from top level `docs` folder within its install directory.

## Quick install procedure example on Ubuntu:
Download eIQ Toolkit install package from [DOWNLOADS section of NXP eIQ website](https://www.nxp.com/design/software/development-software/eiq-ml-development-environment/eiq-toolkit-for-end-to-end-model-development-and-deployment:EIQ-TOOLKIT#downloads).
Type `1.10.0` on the Downloads search bar and select the eIQ Toolkit 1.10.0 Installer for Ubuntu.
No additional Extension is needed.<br>
Login to an NXP account is required to start the download, it will be possible to create one directly when the installer is selected.

Then open a terminal and enter following commands:
```bash
# paths below to be adapted according to package version
# make package executable and execute with root privilege
$ chmod a+x ~/Downloads/eiq-toolkit-v1.10.0.57-1_amd64_b240116.deb.bin
$ sudo ~/Downloads/eiq-toolkit-v1.10.0.57-1_amd64_b240116.deb.bin
```

It will be asked to agree with the licence agreement.
Then some extensions to be installed are proposed,
none of them is necessary for the rest of the download procedure.

## eIQ Toolkit environment setup

eIQ Toolkit environment configures specific python interpreter and packages to be used. It shall be done only once within same shell session:
```bash
# adapt base path according to your install
$ EIQ_BASE="/opt/nxp/eIQ_Toolkit_v1.10.0"
$ EIQ_ENV="${EIQ_BASE}/bin/eiqenv.sh"
$ source "${EIQ_ENV}"
```

## Add supplementary packages
Additional packages shall be installed on top of eIQ Portal after [setting the eIQ environment](#eiq-toolkit-environment-setup) if not already done.
```bash
# Executed from eIQ environment-enabled shell
# cleanup obsolete eIQ python userbase packages to avoid conflicts from past installs 
$ rm -rf "${PYTHONUSERBASE}"
# install required packages in the nxp-nnstreamer-examples downloads directory
$ python -m pip install -r <nxp-nnstreamer-examples>/downloads/requirements.txt
```

# Jupyter Notebook setup
Notebook will make use of eIQ Toolkit tools, therefore Jupyter must be started after [setting the eIQ environment](#eiq-toolkit-environment-setup) if not already done.

```bash
# Executed from eIQ environment-enabled shell
# go to the nxp-nnstreamer-examples downloads directory
$ cd <nxp-nnstreamer-examples>/downloads
# Startup Jupyter Notebook web application
$ jupyter notebook
# or alternatively use
$ python -m notebook
```
To run the notebook, from the Jupyter Notebook web page use browser to open [download.ipynb](./download.ipynb).
Within this notebook, execute all sections by pressing the `▶▶` button.