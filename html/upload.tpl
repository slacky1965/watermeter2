<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0" charset=utf-8>
<meta charset="UTF-8">
<title>%platform%. %modulename%</title>
<style type="text/css">
@media (max-width: 1024px) {
  div#main {
    width: 100%%;
    height: auto;
    }
}
@media (max-width: 768px) {
  div#main {
    width: 100%%;
    height: auto;
  }
}
@media (max-width: 480px) {
  div#main {
    width: 100%%;
    height: auto;
  }
}
@media (max-width: 320px) {
  div#main {
    width: 100%%;
    height: auto;
  }
}
</style></head>
<body>
<p>Please upload %upgfile% file<p>
<form method='POST' action='/update' enctype='multipart/form-data'>
<input type='file' name='update'>
<input type='submit' value='Update'></form>
<p><a href="config">Return to Config</a></p>
</body>
</html>
