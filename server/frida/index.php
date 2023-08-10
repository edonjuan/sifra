<?php
include 'conexion.php';
/* 
  SELECT (hora_registro AT time ZONE 'utc') AT time ZONE 'cdt' AS hora_registro
FROM mi_tabla
  */

//Casos posibles para filtrar
/*
1. Seleccionar solo la matricula.
2. Seleccionar matricula y una fecha.
3. Seleccionar matricula y dos fecha.

4. Seleccionar un fecha sin seleccionar la matricula.
5. Seleccionar dos fechas sin seleccionar matricula.
*/

if (isset($_POST['Filtrar'])) {
  $matricula = $_POST['Matricula'];
  $fechaInicio = $_POST['FechaInicio'];
  $fechaFinal = $_POST['FechaFinal'];
  // Caso 1
  if ($matricula != "" and $fechaInicio == "" and $fechaFinal == "") {
    $matricula = $_POST['Matricula'];
    $filter = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           where matricula like '%$matricula%'
           order by fecha DESC";
    $query = mysqli_query($connect, $filter);
  }
  // Caso 2 
  if ($matricula != "" and ($fechaInicio != "" or $fechaFinal != "") and !($fechaInicio != "" and $fechaFinal != "")) {
    if ($fechaInicio != "") {
      $fecha = $fechaInicio;
    } else {
      $fecha = $fechaFinal;
    }
    $filter = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           where matricula like '%$matricula%' and DATE(fecha) = '$fecha'
           order by fecha DESC";
    $query = mysqli_query($connect, $filter);
  }
  // Caso 3
  // Validar fechas

  $fecha1 = strtotime($fechaInicio);
  $fecha2 = strtotime($fechaFinal);
  if ($fecha1 > $fecha2) {
    echo '
      <script>
        alert("La fecha Inicial no puede ser mayor a la fecha final");
        window.location="index.php";
      </script>
    ';
  }
  if ($matricula != "" and $fechaInicio != "" and $fechaFinal != "") {
    $filter = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           where matricula like '%$matricula%' and DATE(fecha) between '$fechaInicio' and '$fechaFinal'
           order by fecha DESC";
    $query = mysqli_query($connect, $filter);
  }
  // Caso 4
  if ($matricula == "" and ($fechaInicio != "" or $fechaFinal != "") and !($fechaInicio != "" and $fechaFinal != "")) {
    if ($fechaInicio != "") {
      $fecha = $fechaInicio;
    } else {
      $fecha = $fechaFinal;
    }
    $filter = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           where DATE(fecha) = '$fecha'
           order by fecha DESC";
    $query = mysqli_query($connect, $filter);
  }
  // Caso 5
  // Validar fechas

  $fecha1 = strtotime($fechaInicio);
  $fecha2 = strtotime($fechaFinal);
  if ($fecha1 > $fecha2) {
    echo '
      <script>
        alert("La fecha Inicial no puede ser mayor a la fecha final");
        window.location="index.php";
      </script>
    ';
  }
  if ($matricula == "" and $fechaInicio != "" and $fechaFinal != "") {
    $filter = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           where matricula like '%$matricula%' and DATE(fecha) between '$fechaInicio' and '$fechaFinal'
           order by fecha DESC";
    $query = mysqli_query($connect, $filter);
  }
  // Caso 6
  if ($matricula == "" and $fechaInicio == "" and $fechaFinal == "") {
    $filter = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           order by fecha DESC";
    $query = mysqli_query($connect, $filter);
  }
} else {
  $data = "SELECT CONVERT_TZ(fecha,'+00:00','-06:00') as fecha, nombre,matricula from bitacoras
           inner join lugar on lugar.id_lugar = bitacoras.id_lugar 
           inner join alumno on bitacoras.id_alumno = alumno.id_alumno
           order by fecha DESC";
  $query = mysqli_query($connect, $data);
}

?>

<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Frida</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-9ndCyUaIbzAi2FUVXJi0CjmCapSmO7SnpJef0486qhLnuZ2cdeRhO02iuK6FUUVM" crossorigin="anonymous">
</head>

<body>
  <div class="container-fluid">
    <div class="container">
      <div class="row">
        <figure class="text-center pt-5">
          <p class="h1">Bitacora Digital</p>
        </figure>
      </div>
      <div class="row mt-n1">
        <form method="POST">
          <div class="row align-items-center">
            <div class="col-2">
              <img src="/img/Innovation Lab Network.png" class="img-fluid" alt="laboratoryLogo" style="height:150px;width:150px">
            </div>
            <div class="col">
              <div class="input-group input-group-sm mb-3">
                <span class="input-group-text" id="inputGroup-sizing-sm">Matricula</span>
                <input type="number" name="Matricula" class="form-control" aria-label="Sizing example input" aria-describedby="inputGroup-sizing-sm">
              </div>
            </div>
            <div class="col">
              <div class="input-group input-group-sm mb-3">
                <span class="input-group-text" id="inputGroup-sizing-sm">Fecha Inicio</span>
                <input type="date" name="FechaInicio" class="form-control" aria-label="Sizing example input" aria-describedby="inputGroup-sizing-sm">
                <span class="input-group-text" id="inputGroup-sizing-sm">Fecha Final</span>
                <input type="date" name="FechaFinal" class="form-control" aria-label="Sizing example input" aria-describedby="inputGroup-sizing-sm">
                <input type="submit" name="Filtrar" value="Filtrar" class="btn btn-outline-primary">
              </div>
            </div>
          </div>
        </form>
      </div>
      <table class="table">
        <thead>
          <tr class="table-primary">
            <th>Fecha</th>
            <th>Matricula</th>
            <th>Lugar </th>
          </tr>
        </thead>
        <tbody>
          <?php while ($row = mysqli_fetch_array($query)) : ?>
            <tr>
              <th><?= $row['fecha'] ?></th>
              <th><?= $row['matricula'] ?></th>
              <th><?= $row['nombre'] ?></th>
            </tr>
          <?php endwhile; ?>
        </tbody>
      </table>
    </div>
  </div>
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js" integrity="sha384-geWF76RCwLtnZ8qwWowPQNguL3RmwHVBC9FhGdlKrxdiJJigb/j/68SIy3Te4Bkz" crossorigin="anonymous"></script>
</body>

</html>
