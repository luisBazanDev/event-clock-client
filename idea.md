La idea principal será tomar este código base para el manejo de la pantalla LCD, pero los datos los traeremos de otro lado.
En la linea 1 o en la linea superior, pondras siempre un texto de "Hackathon ManaTI"
En la linea de abajo pondras un reloj atras con este formato "T-HH:mm:ss ~1d" y ese "~1d" sera el tiempo aproximado, 2d, 1d, 23h, 22h, 21h, ..., 2h, 1h, <1h, GG! ten encuenta la longitud y que la pantalla tiene 16 caracteres de ancho.

Una vez el contador llegue a 0 deberá decir en la línea superior "Good job!" centrado y "ManaTI Boys" en la línea de abajo.

La fecha límite para el contador, o la fecha y hora para que llegue a 0 sera el 8 de noviembre a las 7:00 p. m hora peru.

Ahora para que lleves la hora dependeras de servidores de hora, que consultaras cada 5 minutos para que lleves una cuenta precisa del tiempo, usa los servers publicos que existen.

El tema de conexiones wifi, deberas guardar la red wifi en tu memoria flash, y si por algun motivo pierdes la señal, o no encuentras la red levantaras una red local desde la que podré conectarme desde mi teléfono con un html donde podré seleccionar la red y colocar la contraseña y la guaradas en la memoria flash de la esp32.
