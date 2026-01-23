# Download examples dependencies
Examples require model files and some associated metadata (labels, ssd priors definitions...) to be downloaded so that they are made available locally to examples in top level `downloads` directory.<br>
This download procedure is to be done once from the host PC. A Jupyter notebook [download.ipynb](./download.ipynb) is available to fetch all dependencies from network and to apply necessary format-conversions.<br>
This Notebook execution requires Jupyter Notebook python package.

# eIQ Neutron SDK install and environment setup

## Quick install procedure example on Ubuntu
To use eIQ Neutron SDK to convert models for Neutron NPU (i.MX95 and i.MX952), python version must be above or equal to `3.10`. To check if your python version is supported, refer to https://eiq.nxp.com/repository.<br>
To install the SDK and set up the environment, follow these steps:

```bash
# Install eIQ Neutron SDK from NXP repository
pip install --index-url https://eiq.nxp.com/repository eiq-neutron-sdk==2.2.2
```

More details on the SDK and its features can be found in the documentation of the eIQ Neutron SDK package which can be downloaded from [NXP eIQ website](https://www.nxp.com/design/design-center/software/eiq-ai-development-environment/eiq-toolkit-for-end-to-end-model-development-and-deployment:EIQ-TOOLKIT).
It is available for Windows and Ubuntu OS platforms.<br>
For more information, refer to eIQ Toolkit User Guide available on [the documentation section of NXP eIQ website](https://www.nxp.com/design/software/development-software/eiq-ml-development-environment/eiq-toolkit-for-end-to-end-model-development-and-deployment:EIQ-TOOLKIT#documentation), and also from top level `docs` folder within its install directory.

## Add supplementary packages
Additional packages shall be installed on top of eIQ Neutron SDK:
```bash
# Install required packages in the nxp-nnstreamer-examples downloads directory
python -m pip install -r <nxp-nnstreamer-examples>/downloads/requirements.txt
```

# Jupyter Notebook setup
Notebook will make use of eIQ Neutron SDK to convert models for Neutron NPU support:
```bash
# Go to the nxp-nnstreamer-examples downloads directory
cd <nxp-nnstreamer-examples>/downloads
# Startup Jupyter Notebook web application
jupyter notebook
# or alternatively use
python -m notebook
```
To run the notebook, from the Jupyter Notebook web page use browser to open [download.ipynb](./download.ipynb).
Within this notebook, execute all sections by pressing the `▶▶` button.
