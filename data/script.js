$(document).ready(function(){
    $.get("/get?connection_type", function(data, status){
        $('#content-network-adapter').html(data);
      });
    $.get("/get?wifi_ip", function(data, status){
        $('#content-wifi-ip').html(data);
      });
    $.get("/get?wifi_mac", function(data, status){
        $('#content-wifi-mac').html(data);
      });
    $.get("/get?eth_ip", function(data, status){
        $('#content-eth-ip').html(data);
      });
    $.get("/get?eth_mac", function(data, status){
        $('#content-eth-mac').html(data);
      });
    $.get("/get?wifi_ssid", function(data, status){
        $('#input-wifi-ssid').val(data);
      });
    $.get("/get?wifi_password", function(data, status){
        $('#input-wifi-password').val(data);
      });
});

function saveSettings()
{
    $.post("/save", {wifi_ssid: $('#input-wifi-ssid').val(), wifi_password:$('#input-wifi-password').val()}).done(function( data ) {
        alert(data);
      });
}