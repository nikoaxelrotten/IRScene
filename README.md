# IRScene

Plugin VST3 de dos cabinas (IR loader) hecho con JUCE 8, pensado para usar en REAPER.

## Qué hace

- Dos slots de IR (A y B). Cargás tus propios archivos `.wav` / `.aif` / `.flac` con el botón de carpeta, con la lista o con las flechas `<` `>` (pasan al IR anterior o siguiente de la carpeta). **No incluye ningún impulso.**
- Por slot: Gain, Hi Pass, Lo Pass, Resonance (Q del Lo Pass), Delay (0 a 20 ms), Panning, inversión de fase (Ø) y solo (S).
- Globales: Balance entre A y B y Room (reverb de ambiente suave).
- Interfaz escalable: arrastrá la esquina inferior derecha o usá los botones de escala (75 / 100 / 125 / 150 %). El tamaño y las rutas de los IRs se guardan con el proyecto.
- Doble clic en un knob o fader: vuelve a su valor por defecto.

## Cómo conseguir el .vst3 (sin instalar nada para compilar)

1. Subí este repositorio a GitHub (ya está si lo ves acá).
2. Pestaña **Actions**: el workflow *Build VST3* corre solo en cada push (o con *Run workflow*).
3. Cuando termine, abrí la ejecución y bajá el artefacto **IRScene-Windows-VST3** (o macOS).
4. Descomprimí y copiá la carpeta `IRScene.vst3` a `C:\Program Files\Common Files\VST3\`.
5. En REAPER: Preferences > Plug-ins > VST > Re-scan (o *Clear cache and re-scan*).

## Compilar en tu PC

Requiere CMake 3.22+ y Visual Studio 2022 (carga de trabajo C++) en Windows:

```
cmake -S . -B build
cmake --build build --config Release
```

El plugin queda en `build/IRScene_artefacts/Release/VST3/IRScene.vst3`.

## Estructura

- `Source/PluginProcessor.*`: parámetros y audio (convolución, filtros, delay, balance, room).
- `Source/PluginEditor.*`: interfaz escalable.
