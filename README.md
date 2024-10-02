# Setting up an AI/ML Workstation with Intel GPU Max 1550 (Ponte Vecchio)

This guide will walk you through the process of setting up an AI/ML workstation using the Intel GPU Max 1550 (Ponte Vecchio). We'll cover the installation of Conda, necessary drivers, and key AI/ML frameworks optimized for Intel hardware.

## 1. System Requirements for this Tutorial 

- A system with Intel GPU Max 1550 (Ponte Vecchio)
- Ubuntu 20.04 or later (recommended)
- Latest Intel GPU Drivers
- OneAPI: [Link](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html?operatingsystem=linux&linux-install-type=apt)
- Sudo Permissions 
- Optional: [Intel AI Tools](https://www.intel.com/content/www/us/en/developer/tools/oneapi/ai-tools-selector.html)

## 2. Install Conda

1. Check if wget is installed:
   ```bash
   which wget
   ```

2. If wget is not installed, install it:
   ```bash
   sudo apt-get update
   sudo apt-get install wget
   ```

3. Download the Miniconda installer:
   ```bash
   wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
   ```

4. Make the installer executable:
   ```bash
   chmod +x Miniconda3-latest-Linux-x86_64.sh
   ```

5. Run the installer:
   ```bash
   ./Miniconda3-latest-Linux-x86_64.sh
   ```

6. Follow the prompts in the installer.

7. After installation, restart your terminal or source your .bashrc file:
   ```bash
   source ~/.bashrc
   ```

## 3. Set Up Conda Environment

1. Create and activate a new conda environment:
   ```bash
   conda create -n intel_ml python=3.10
   conda activate intel_ml
   ```

2. Install AI/ML packages optimized for Intel hardware:
   ```bash
   conda install -c https://software.repos.intel.com/python/conda -c conda-forge --override-channels intel-extension-for-tensorflow=2.15=*cpu* intel-extension-for-pytorch=2.3.100 oneccl_bind_pt=2.3.0 torchvision=0.18.1 torchaudio=2.3.1 deepspeed=0.14.2
   ```

## 4. Verify Installation

To verify that the AI tools are properly installed, use the following commands:

- Intel® Extension for PyTorch* (GPU):
  ```bash
  python -c "import torch; import intel_extension_for_pytorch as ipex; print(torch.__version__); print(ipex.__version__); [print(f'[{i}]: {torch.xpu.get_device_properties(i)}') for i in range(torch.xpu.device_count())];"
  ```

- Intel® Extension for TensorFlow* (GPU):
  ```bash
  python -c "from tensorflow.python.client import device_lib; print(device_lib.list_local_devices())"
  ```

## 6. Optional: Install Additional AI/ML Tools

- Intel® Optimization for XGBoost*:
  ```bash
  conda install -c conda-forge xgboost
  ```

- Intel® Extension for Scikit-learn*:
  ```bash
  conda install scikit-learn-intelex
  ```

- Modin*:
  ```bash
  conda install -c conda-forge modin
  ```

- Intel® Neural Compressor:
  ```bash
  pip install neural-compressor
  ```

## 7. Start Developing

You're now ready to start developing AI/ML applications using your Intel GPU Max 1550. Remember to activate your conda environment before starting your work:

```bash
conda activate intel_ml
```

Happy coding!
