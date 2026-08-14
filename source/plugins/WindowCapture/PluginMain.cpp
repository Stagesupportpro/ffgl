// PluginMain.cpp
//
// Registra el plugin ante Resolume: nombre, ID único de 4 caracteres,
// tipo (fuente, no efecto) y la función que crea instancias.
//
// [SDK-CHECK] La firma exacta del constructor de CFFGLPluginInfo ha variado
// entre versiones del SDK (algunos snapshots añaden/quitan campos como el
// número de versión de API). Compara con el ejemplo "Gradients" de tu copia
// local de github.com/resolume/ffgl y ajusta el orden/número de argumentos
// si el compilador se queja aquí — es la última pieza específica de versión.

#include "WindowCaptureSource.h"

static CFFGLPluginInfo PluginInfo(
	WindowCaptureSource::CreateInstance, // función de creación de instancias
	"WNCP",                              // ID único de 4 caracteres (elige el tuyo)
	"Window Capture",                    // nombre mostrado en Resolume
	1,                                   // versión mayor de API
	500,                                 // versión menor de API
	1,                                   // versión mayor del plugin
	0,                                   // versión menor del plugin
	FF_SOURCE,                           // tipo: fuente (no tiene entradas de vídeo)
	"Captura una ventana de Windows como fuente de vídeo",
	"Selecciona la ventana a capturar en el parámetro 'Window'"
);
