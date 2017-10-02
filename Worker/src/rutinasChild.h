#ifndef RUTINAS_CHILD_H_
#define RUTINAS_CHILD_H_

/* Es el Worker hijo que realiza la ejecucion de Transformacion o Reducciones.
 */
void subrutinaEjecutor(int sock_master);

/* Es el Worker hijo que funciona de server y provee su FILE al Worker cliente
 * para que realice la Reduccion Global.
 */
void subrutinaServidor(int sock_worker);

#endif /* RUTINAS_CHILD_H_ */
