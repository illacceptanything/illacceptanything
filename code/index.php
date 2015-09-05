<!DOCTYPE html>
<html>
<head lang="en">
    <meta charset="UTF-8">
    <title>Anything you want</title>
    <script src="https://code.jquery.com/jquery-2.1.3.min.js"></script>
    <script src="http://code.onion.com/fartscroll.js"></script>
    <script type="text/javascript">
        jQuery(document).ready(function () {
            fartscroll(200);
        });
    </script>
    <style type="text/css">
        body {
            min-height: 2000px;
        }
    </style>
</head>
<body>
<?php

//TODO:: add some OOP

\Ass\FartGenerator::generate();

?>
<!-- best lyrics ever -->
<iframe width="560" height="315" src="https://www.youtube.com/embed/2HQaBWziYvY?autoplay=1" frameborder="0"
        allowfullscreen></iframe>
</body>
</html>
