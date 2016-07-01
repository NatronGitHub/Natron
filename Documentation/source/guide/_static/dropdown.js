$(document).ready(function() {
    $('.toggleInputTable').click(function(e) {
    $('.inputTable').toggle('slow');
    e.preventDefault();
    });
    $('.toggleControlTable').click(function(e) {
    $('.controlTable').toggle('slow');
    e.preventDefault();
    });
})
