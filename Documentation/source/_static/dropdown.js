$(document).ready(function () {
$('ul li a').click(function(event){
$(this).siblings('ul').slideToggle('slow');
});
});
