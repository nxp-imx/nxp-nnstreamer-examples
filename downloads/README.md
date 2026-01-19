# Download examples dependencies
Examples require model files and some associated metadata (labels, ssd priors definitions...) to be downloaded so that they are made available locally to examples in top level `downloads` directory.<br>
This download procedure is to be done once from the host PC. A Jupyter notebook [download.ipynb](./download.ipynb) is available to fetch all dependencies from network and to apply necessary format-conversions.<br>
This Notebook execution requires Jupyter Notebook python package.

# eIQ Neutron SDK install and environment setup
To use eIQ Neutron SDK to convert models for Neutron NPU (i.MX95 and i.MX952), NXP provides a python wheel that can be installed. However, more details on the SDK and its features can be found in the documentation of the eIQ Neutron SDK package which can be downloaded from [NXP eIQ website](https://www.nxp.com/design/design-center/software/eiq-ai-development-environment/eiq-toolkit-for-end-to-end-model-development-and-deployment:EIQ-TOOLKIT).
It is available for Windows and Ubuntu OS platforms.<br>
For more information, refer to eIQ Toolkit User Guide available on [the documentation section of NXP eIQ website](https://www.nxp.com/design/software/development-software/eiq-ml-development-environment/eiq-toolkit-for-end-to-end-model-development-and-deployment:EIQ-TOOLKIT#documentation), and also from top level `docs` folder within its install directory.

# Jupyter Notebook setup
Notebook will make use of eIQ Neutron SDK to convert models for Neutron NPU support. Before running the notebook, ensure the eIQ Neutron SDK wheel is installed in your Python environment.
```bash
# go to the nxp-nnstreamer-examples downloads directory
cd <nxp-nnstreamer-examples>/downloads
## install Jupyter Notebook package
pip install jupyter==1.0.0
# Startup Jupyter Notebook web application
jupyter notebook
# or alternatively use
python -m notebook
```
To run the notebook, from the Jupyter Notebook web page use browser to open [download.ipynb](./download.ipynb).
Within this notebook, execute all sections by pressing the `▶▶` button.
