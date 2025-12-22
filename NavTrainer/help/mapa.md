# Herramientas en Mapa

## Introducción a las herramientas

NavTrainer proporciona un conjunto completo de herramientas digitales que simulan los instrumentos tradicionales de navegación. Estas herramientas te permiten trabajar con cartas náuticas de manera precisa y eficiente.

## Navegación por el mapa

### Desplazamiento

Puedes moverte por la carta náutica de las siguientes formas:

- **Modo Desplazamiento (Drag)**: Selecciona el modo "Desplazamiento" en el panel flotante (o pulsa **D**) y luego haz clic y arrastra con el botón izquierdo para desplazar la carta.
- **Arrastrar widgets**: La barra de herramientas flotante solo puede moverse arrastrando su **barra de título**.
- **Teclas de dirección**: Usa las flechas del teclado para movimientos precisos

### Zoom

Ajusta el nivel de zoom para ver más o menos detalle:

- **Rueda del ratón (con Shift)**: Mantén presionada la tecla **Shift** mientras usas la rueda del ratón para acercar/alejar el mapa (por diseño, la rueda sola no hace zoom).
- **Atajos de teclado**: Usa los atajos configurados (por defecto hay accesos rápidos para otros modos; el zoom principal usa Shift+rueda)

## Herramientas de medición

### Regla

La regla te permite medir distancias y orientarla libremente sobre la carta.

**Cómo usar la regla:**

1. Arrastra la Regla desde el panel de herramientas hasta la carta (soltar donde quieras). La regla se coloca centrada en el punto de soltar.
2. Para mover la regla: haz clic en cualquier parte de la regla (excepto en el asa derecha) y arrastra.
3. Para rotarla: arrastra el asa derecha (círculo verde).

### Transportador

El transportador digital te permite medir ángulos y rumbos con precisión.

**Cómo usar el transportador:**

1. Selecciona la herramienta "Transportador"
2. Haz clic en el centro del transportador
3. Arrastra para orientar el transportador
4. Lee el ángulo en la escala circular

### Compás

El compás sirve tanto para trazar círculos de posición como para dibujar arcos (derrotas) con la punta del lápiz.

**Interacciones principales:**

- **Mover**: Arrastra el centro del compás (área de la bisagra) para moverlo por la carta.
- **Rotación total**: Arrastra la punta pivot (extremo metálico) o la punta de lápiz para rotar todo el compás (la punta pivot se mantiene fija en coordenadas del mapa durante la rotación).
- **Rotación desde el lápiz (dibujar arcos)**: Si arrastras desde la punta del lápiz con el botón izquierdo, el compás rota y simultáneamente **dibuja un arco** en el mapa (previsualización en tiempo real). Si usas el botón derecho para rotar desde el lápiz, el compás rota sin dibujar el arco.

**Cómo trazar un arco permanente:**

1. Coloca el compás en la posición deseada (la punta pivot es el centro del arco).
2. Haz clic y arrastra desde la punta del lápiz (botón izquierdo) para definir la extensión; verás una previsualización.
3. Suelta para fijar el arco; si el barrido es mayor a un pequeño umbral, la aplicación añadirá una anotación de arco.

## Herramientas de dibujo

### Herramienta de punto

Marca posiciones importantes en la carta.

**Uso:**

1. Selecciona "Punto"
2. Haz clic en la ubicación deseada
3. Selecciona un color para el punto mediante Clic derecho > Cambiar color... (si procede)

### Herramienta de línea

Traza líneas de rumbo, demarcaciones o cualquier otra línea recta.

**Uso:**

1. Selecciona "modo dibujo"
2. Añade la regla en el mapa
3. Situa la regla en la orientación deseada
4. Traza la recta ayudandote de la regla

**Opciones:**

- Grosor de línea
- Color personalizable

### Herramienta de texto

Añade anotaciones de texto a la carta.

**Uso:**

1. Selecciona "Texto"
2. Haz clic donde quieras colocar el texto
3. Escribe tu anotación
4. Ajusta formato (color, opacidad) mediante Clic derecho > Cambiar color... (si procede)

## Gestión de elementos

### Seleccionar elementos

Usa la herramienta "Seleccionar" (cursor) para:

- Seleccionar un elemento individual: Haz clic derecho en él

### Editar elementos

Con elementos seleccionados puedes:

- **Modificar**: Haz doble clic para editar propiedades
- **Eliminar**: Pulsa Supr o usa el botón "Eliminar"

### Deshacer

Si cometes un error:

- **Deshacer**: Ctrl+Z o botón "Deshacer"
- Historial de 50 acciones

### Atajos de teclado

Atajos útiles para acceso rápido (configurables):

- **D**: Activar modo *Drag* (desplazamiento)
- **P**: Activar modo *Paint* (dibujo)
- **E**: Activar modo *Erase* (borrador)
- **T**: Activar Transportador
- **C**: Activar Compás
- **P**: Añadir punto (cuando el modo lo permita)
- **L**: Dibujar línea
- **A**: Añadir texto
- **Esc**: Cancelar operación actual
- **Del**: Eliminar elementos seleccionados

## Importar mapas personalizados
Puedes cargar tus propias cartas náuticas en formato imagen (JPG, PNG):

1. Ve a "Archivo (icono de carpeta)" > Seleccionar mapa personalizado
2. Elige la imagen desde tu disco