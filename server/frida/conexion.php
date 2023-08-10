<?php
    $host_name="127.0.0.1";
    $database="rfid";
    $user_name="root";
    $psw="admin#2022";

    //conectar a la base de datos utilizando mysqli_connect
    $connect = mysqli_connect($host_name,$user_name,$psw,$database);
/*
    //validación
    if(mysqli_connect_errno()){
        die('<p>Failed to connect to MySQL: '.mysqli_connect_errno().'</p>');
    }else{
        print_r("Conexión con la base de datos exitoso");
    }
*/ 
?>