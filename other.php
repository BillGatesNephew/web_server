<!doctype html>
<html>
    <head>
        <title>My Other Page</title>
    </head>
    <body>
        <h2>My Other Page's Text</h2>
        <ul>
            <?php
                $test = ["one" => 1, "two" => 2, "three" => 3];
                foreach($test as $key => $val) {
            ?>
                <li><?= $key ?>:<?= $val ?></li>
                <?php }?>
        </ul>
    </body>
</html>