<!DOCTYPE html>
<html>
<head lang="en">
    <meta charset="UTF-8">
    <title>Any thing you want</title>
	<script src="https://code.jquery.com/jquery-2.1.3.min.js"></script>
        <script src="http://code.onion.com/fartscroll.js"></script>
        <script type="text/javascript">
                jQuery(document).ready(function(){
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
try {
    echo 'fart'; 
} catch (PoopException $e) {
    echo 'Caught exception: ',  $e->getMessage(), "\n";
}
?>
</body>
</html>
