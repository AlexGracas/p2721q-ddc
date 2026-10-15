# p2721q-ddc

**EN** · [PT-BR](#português)

A small command-line DDC/CI driver for the **Dell P2721Q** on **macOS (Apple Silicon)**, written in C. It controls brightness, contrast, input, preset and color settings, and includes a switch for the macOS dithering that is commonly linked to the monitor's flickering on M-series Macs.

## Install

Download `p2721q-1.0.0.pkg` from the [Releases](../../releases) page and open it (it installs `/usr/local/bin/p2721q`). The package is unsigned: on first open, right-click → Open, or allow it in System Settings → Privacy & Security.

Or build from source (needs Xcode Command Line Tools):

```
make
sudo make install
```

## Usage

```
p2721q get                     # current brightness
p2721q set 50                  # brightness 0-100
p2721q up [step] | down [step] # adjust brightness (default 10)
p2721q get contrast            # read a feature (name or 0xNN VCP code)
p2721q set contrast 60         # write a feature
p2721q set input 0x0F          # input: 0x1B USB-C, 0x0F DisplayPort, 0x11 HDMI
p2721q set preset 5            # 0 Standard, 3 Movie, 5 Game
p2721q caps                    # official capabilities string of the monitor
p2721q dither [on|off]         # macOS temporal dithering (all displays)
```

Names: `brightness contrast input preset colortemp red green blue power`.

Requirements: Apple Silicon Mac, **DDC/CI enabled** in the monitor menu (Menu → Others → DDC/CI).

## Flickering on Apple Silicon

Dell lists the P2721Q as affected by flicker with Apple M chips ([Dell KB](https://www.dell.com/support/kbdoc/en-us/000331897/apple-m-processors-might-cause-dell-monitors-to-flicker)). Reported mitigations: `p2721q dither off` (same technique as [Stillcolor](https://github.com/aiaf/Stillcolor); resets on reboot), setting the monitor's *Input Color Format* to RGB in its on-screen menu, and a fixed 60 Hz refresh rate. Results vary.

## Caveats

- Uses the private `IOAVService` API, which may change in future macOS versions.
- `set input` to an input with no signal blanks the screen; switch back from the monitor's physical menu.
- This tool does not flash firmware. Provided "as is", without warranty. Not affiliated with Dell.

## License

MIT

---

## Português

Um pequeno driver de linha de comando DDC/CI para o **Dell P2721Q** no **macOS (Apple Silicon)**, escrito em C. Controla brilho, contraste, entrada, preset e cores, e inclui um interruptor para o dithering do macOS, associado ao flicker do monitor em Macs com chip M.

### Instalação

Baixe o `p2721q-1.0.0.pkg` na página de [Releases](../../releases) e abra (instala `/usr/local/bin/p2721q`). O pacote não é assinado: na primeira vez, clique com o botão direito → Abrir, ou permita em Ajustes do Sistema → Privacidade e Segurança.

Ou compile do código-fonte (requer Xcode Command Line Tools):

```
make
sudo make install
```

### Uso

```
p2721q get                     # brilho atual
p2721q set 50                  # brilho 0-100
p2721q up [passo] | down [passo]
p2721q get contrast            # lê um recurso (nome ou código VCP 0xNN)
p2721q set contrast 60         # escreve um recurso
p2721q set input 0x0F          # entrada: 0x1B USB-C, 0x0F DisplayPort, 0x11 HDMI
p2721q set preset 5            # 0 Standard, 3 Movie, 5 Game
p2721q caps                    # capabilities oficiais do monitor
p2721q dither [on|off]         # dithering temporal do macOS (todos os displays)
```

Requisitos: Mac Apple Silicon e **DDC/CI ativado** no menu do monitor (Menu → Others → DDC/CI).

### Flicker no Apple Silicon

A Dell lista o P2721Q como afetado por flicker com chips Apple M ([KB da Dell](https://www.dell.com/support/kbdoc/en-us/000331897/apple-m-processors-might-cause-dell-monitors-to-flicker)). Mitigações relatadas: `p2721q dither off` (mesma técnica do [Stillcolor](https://github.com/aiaf/Stillcolor); volta ao reiniciar), configurar *Input Color Format* para RGB no menu do monitor e usar 60 Hz fixos. Os resultados variam.

### Avisos

- Usa a API privada `IOAVService`, que pode mudar em versões futuras do macOS.
- `set input` para uma entrada sem sinal deixa a tela preta; volte pelo menu físico do monitor.
- Esta ferramenta não grava firmware. Fornecida "como está", sem garantia. Sem vínculo com a Dell.

### Licença

MIT
