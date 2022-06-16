# Download examples dependencies
Examples require model files and some associated metadata (labels, ssd priors definitions...) to be downloaded so that they are made available locally to examples in top level `downloads` directory.<br>
This download procedure is to be done once from the host PC. A Jupyter notebook [download.ipynb](./download.ipynb) is available to fetch all dependencies from network and to apply necessary format-conversions.<br>
This Notebook execution requires:
1. eIQ Toolkit local install
2. Jupyter Notebook python package

# eIQ Toolkit install and environment setup
Last version of eIQ Toolkit installer can be downloaded from [NXP eIQ website](https://www.nxp.com/eiq).
It is available for Windows and Ubuntu OS patforms.
For more information, refer to eIQ Toolkit User Guide available from top level `docs` folder within its install directory.

## Quick install procedure example on Ubuntu:
Download eIQ Toolkit install package from [NXP eIQ website](https://www.nxp.com/eiq) then enter commands:
```bash
# paths below to be adapted according to package version
# make package executable and execute with root privilege
$ chmod a+x ~/Downloads/eiq-toolkit-v1.4.5.61-1_amd64_b220712.deb.bin
$ sudo ~/Downloads/eiq-toolkit-v1.4.5.61-1_amd64_b220712.deb.bin

# eIQ Toolkit fixups - adapt base path according to your install
$ EIQ_BASE="/opt/nxp/eIQ_Toolkit_v1.4.5"
$ EIQ_ENV="${EIQ_BASE}/bin/eiqenv.sh"

# fixup eIQ directories access rights
$ sudo find "${EIQ_BASE}" -type d -exec chmod a+rx {} \;
# fixup toco_from_protos python shebang (use ''#!/usr/bin/env python')
$ sudo sed -i 's/#!\/python\/bin\/python/#!\/usr\/bin\/env python/' "${EIQ_BASE}"/python/bin/toco_from_protos
# fixup eIQ env to include binaries from eIQ python userbase packages
$ sudo bash -c "echo -e '\nexport PATH=\"\${PYTHONUSERBASE}/bin:\${PATH}\"' >> \"${EIQ_ENV}\""
```

## eIQ Toolkit environment setup
eIQ Toolkit environment configures specific python interpreter and packages to be used. It shall be done only once within same shell session:
```bash
# adapt base path according to your install
$ EIQ_BASE="/opt/nxp/eIQ_Toolkit_v1.4.5"
$ EIQ_ENV="${EIQ_BASE}/bin/eiqenv.sh"
$ source "${EIQ_ENV}"
```

## Add supplementary packages
Additional packages (jupyter, vela) shall be installed on top of eIQ Portal after from a shell after [setting the eIQ environment](#eiq-toolkit-environment-setup) if not already done.
```bash
# cleanup obsolete eIQ python userbase packages to avoid conflicts from past installs 
$ rm -rf "${PYTHONUSERBASE}"

# add jupyter package
$ python -m pip install jupyter

# clone and install (NXP version) of vela
# Public github hosting NXP version
$ git -C /tmp clone "https://github.com/nxp-imx/ethos-u-vela.git"
# ignore pip's dependency resolver errors on deepview-validator / deepview-datastore...
$ python -m pip install /tmp/ethos-u-vela
$ rm -rf /tmp/ethos-u-vela
```

# Jupyter Notebook setup
Notebook will make use of eIQ Toolkit tools, therefore Jupyter must be started after [setting the eIQ environment](#eiq-toolkit-environment-setup) if not already done.

```bash
# Executed from eIQ environment-enabled shell
# Startup Jupyter Notebook web application
$ cd /path/to/nxp-nnstreamer-examples/downloads
$ jupyter notebook
# or alternatively use
$ python -m notebook
```
To run the notebook, from the Jupyter Notebook web page use browser to open [download.ipynb](./download.ipynb).
Within this notebook, execute all sections by pressing the `>>` button.