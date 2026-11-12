# Janitzio en la Noche de Muertos

Recorrido virtual en C++ y OpenGL 3.3. La isla incluye el embarcadero, los barrios, la ofrenda al pie de Morelos y dos animaciones principales, la procesión con veladoras y la llegada de las lanchas.

## Compilar en Windows

1. Descarga o clona el repositorio completo desde la rama `entrega-janitzio`. Conserva `ConfigInicial/` y `Janitzio/` en la misma carpeta. Las bibliotecas y los encabezados que usa el proyecto están en `ConfigInicial/`.
2. En **Visual Studio Installer**, instala **Desarrollo para el escritorio con C++**, las herramientas de MSVC para **x86/x64** y un SDK de Windows 10 u 11. El proyecto usa el compilador predeterminado de tu instalación, normalmente `v143` en Visual Studio 2022 o `v145` en Visual Studio 2026.
3. Abre `Janitzio/Janitzio.sln`, selecciona **Debug · x86**, compila y pulsa **F5**. Para iniciar fuera de Visual Studio, abre `Janitzio/build/Janitzio.exe` desde su carpeta, junto a `glew32.dll` y `Shader/`. El equipo debe admitir OpenGL 3.3.

También puedes compilar desde **Developer PowerShell for Visual Studio**, en la raíz del repositorio.

```powershell
msbuild .\Janitzio\Janitzio.sln /t:Build /p:Configuration=Debug /p:Platform=x86
```

Si una copia anterior muestra **MSB8020** porque busca `v143`, descarga la versión actual de la rama y vuelve a abrir la solución. Si el error persiste, revisa que estén instaladas las herramientas de C++ para x86/x64. Si Windows informa que falta `MSVCP140.dll` o `VCRUNTIME140.dll` al abrir el ejecutable, instala el [redistribuible de Visual C++ x86 de Microsoft](https://learn.microsoft.com/es-es/cpp/windows/latest-supported-vc-redist).

## Ver la isla con los modelos completos

El proyecto compila con los archivos de Git. Al ejecutar, Janitzio usa geometría de respaldo cuando no carga el paquete de modelos. En esa versión, las casas, los árboles y las lanchas estáticas pueden verse distintos. Las dos animaciones siguen disponibles.

Para ver la escena de la demostración, consigue del equipo los dos archivos preparados, colócalos en estas rutas y vuelve a compilar:

```text
Janitzio/
└── Assets/
    ├── modelos.bin
    └── materiales.png
```

Visual Studio copia ambos a `Janitzio/build/Assets/`. Si ya tienes el ejecutable, también puedes colocarlos en la carpeta `Assets/` junto a él y reiniciar el programa. La consola debe mostrar **Viviendas importadas: 438 de 438** cuando se carga el paquete.

`Assets/` queda fuera de Git. Los `.obj` originales se usan para generar el paquete con `Janitzio/preparar_modelos.py`. La aplicación usa los dos archivos preparados. Consulta los [créditos de los modelos](Janitzio/CREDITS.md) antes de redistribuirlos.

## Controles

- **WASD y ratón** para recorrer la isla. **G** alterna entre caminar y volar.
- **1–9** para cambiar de vista. **2** lleva al muelle y **9** muestra la ofrenda desde arriba.
- **P** inicia o repite la procesión. **L** inicia o repite la llegada de las lanchas.
- **N** alterna entre día y noche. **Esc** cierra el recorrido.
