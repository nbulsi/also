# Mixed Structural Choice Operator: Enhancing Technology Mapping with Heterogeneous Representations (DAC'25)
[![arXiv](https://img.shields.io/badge/arXiv-2504.12824-b31b1b.svg)](https://arxiv.org/abs/2504.12824)

**Mixed Structural Choice** combines different logic representations to enhance the quality of technology mapping results. For a detailed introduction to the method, please refer to our [DAC'25 paper](https://arxiv.org/abs/2504.12824). 

The **Mixed Structural Choice Operator** is integrated into **[ALSO](https://github.com/nbulsi/also.git)** and is represented by the **command `mch`**. Please make sure ALSO is successfully compiled and can run smoothly before using it. Please refer to the [README](README.md) for how to compile ALSO.

The `mch` command in ALSO performs logic optimization and technology mapping using **Mixed Structural Choice**. It supports multiple logic network types (AIG, XMG, MIG, XAG) and mapping targets (Standard Cell, LUT), and the combination of different logic representations can be enabled by the `--MCH` option.

---

## 🔧 Usage

The `mch` command in ALSO performs logic optimization and technology mapping using Mixed Structural Choice. It supports multiple logic network types (AIG, XMG, MIG, XAG) and mapping targets (Standard Cell, LUT), and the combination of different logic representations can be enabled by the `--MCH` option. Use the **`mch -h`** command in the ALSO main page to view the detailed usage of mixed structural choice operator.

```bash
also> mch [OPTIONS] [cut_limit] [cut_size]
```

### ⚙️ Options

| Option                | Description                                                  |
| --------------------- | ------------------------------------------------------------ |
| `-o`, `--output` FILE | Output filename for mapped Verilog                           |
| `-S`, `--Standard`    | Perform Standard Cell Mapping                                |
| `-L`, `--LUT`         | Perform LUT Mapping                                          |
| `-O`, `--opt`         | Perform logic optimization based on exact map                |
| `-a`, `--aig`         | Use AIG network for optimization and mapping                 |
| `-x`, `--xmg`         | Use XMG network for optimization and mapping                 |
| `-g`, `--xag`         | Use XAG network for optimization and mapping                 |
| `-m`, `--mig`         | Use MIG network for optimization and mapping                 |
| `-M`, `--MCH` UINT    | Perform mapping using **Mixed Structural Choice** (see below) |
| `-A`, `--area`        | Area-oriented mapping (skip delay optimization rounds)       |
| `-r`, `--ratio` FLOAT | Critical path node ratio (default: 0.8)                      |
| `-v`, `--verbose`     | Enable detailed runtime information                          |

### 🧾 Positional Arguments

| Argument    | Description                                       |
| ----------- | ------------------------------------------------- |
| `cut_limit` | Maximum number of cuts to enumerate (default: 49) |
| `cut_size`  | Maximum size of each cut (default: 6)             |

### 🚦 MCH Mode Guide (`--MCH`)

You can use the `--MCH` (`-M`) option to select different combinations of logic representations for optimization and mapping. It combines different logic representations to obtain better quality of results.

| Value | Description         |
| ----- | ------------------- |
| `0`   | AIG + XMG (default) |
| `1`   | AIG + XAG           |
| `2`   | AIG + MIG           |
| `3`   | MIG + XMG           |

### 📥 Supported Input Formats

**✅Recommended format: AIGER (.aig)**

Use `read_aiger` in ALSO to load AIG file.

```bash
also> read_aiger my_design.aig
```

------

**🛠️ Convert Verilog/BLIF/Mapped Verilog to AIG**

You can use **[Yosys](https://github.com/YosysHQ/yosys.git)** or **[ABC](https://github.com/berkeley-abc/abc.git)** to convert other formats to AIG:

**🔁 Verilog/BLIF → AIG (Yosys):**

```bash
yosys -p "read_verilog my_design.v/read_blif my_design.blif; hierarchy -auto-top; flatten; proc; opt; techmap; abc -g AND; write_aiger my_design.aig"
```

**🔁 Verilog/BLIF → AIG (ABC):**

```bash
abc -q "read my_design.v/read my_design.blif; strash; write_aiger my_design.aig"
```

If you have already deployed **Yosys** or **ABC** locally, you are welcome to use our scripts `also/test/convert_to_aig.sh` to perform unified format conversion. Then load the AIG in ALSO.

------

### 📤 Output Formats

- #### Standard Cell Mapping Output (Verilog)

Use the `-o` or `--output` option to specify the output path:

```bash
also> mch --aig --Standard -o output_path/mapped.v
```

This generates a standard cell level Verilog netlist based on the technology library.

------

- #### LUT Mapping Output (BLIF)

Use the `write_blif -l` command to save the LUT-mapped result:

```bash
also> write_blif -l output_path/output.blif
```

------

- #### Optimized Logic Output

You can output the technology-independent logic network optimized by `mch`:

- **Optimized AIG**:

  ```bash
  also> write_aiger output_path/optimized.aig
  ```

- **Optimized XMG / MIG / XAG as Verilog**:

  ```bash
  also> write_verilog output_path/optimized.v
  ```


---

## ✅ Examples

#### 1. Standard Cell Mapping (AIG + XMG)

```bash
also> read_aiger example.aig
also> read_genlib asap7.genlib
also> mch -M 0 -S -o mapped.v
```

#### 2. LUT Mapping (AIG + XAG)

```bash
also> read_aiger design.aig
also> mch -M 1 -L -k 6
also> write_blif -l design_lut.blif
```

#### 3. Optimize AIG

```bash
also> read_aiger raw.aig
also> mch -a -O
also> write_aiger optimized.aig
```

## 📚 References

If you use Mixed Structural Choice in your research, please cite our paper:

```bibtex
@article{hu2025mixed,
  title={Mixed Structural Choice Operator: Enhancing Technology Mapping with Heterogeneous Representations},
  author={Hu, Zhang and Pan, Hongyang and Xia, Yinshui and Wang, Lunyao and Chu, Zhufei},
  journal={arXiv preprint arXiv:2504.12824},
  year={2025}
}
```

---

## 📧 Contact

For questions or feedback, please reach out to the authors listed in the [paper](https://arxiv.org/abs/2504.12824) or open an issue in this repository.
