<?php

// $_POST['repo'] - Name of the repository ('head' or 'car')
// $_POST['image_name'] - Base filename of the image
// $_POST['rotation'] - Rotation angle, in degrees COUNTER-clockwise

require_once('inc/photo-config.inc');
require_once('inc/permissions.inc');

start_response();

if (!have_permission(ASSIGN_RACER_IMAGE_PERMISSION)) {
  not_authorized_body();
} else {
  $image_name = $_POST['image_name'];
  $photo_repository = photo_repository(isset($_POST['repo']) ? $_POST['repo'] : 'head');
  $rotation = $_POST['rotation'];

  $original = $photo_repository->lookup(RENDER_ORIGINAL);

  $source_file_path = $original->file_path($image_name);

  list($source_width, $source_height, $image_type) = getimagesize($source_file_path);

  $im = open_image($source_file_path, $image_type);

  $rot = imagerotate($im, $rotation, /* fill color */ 0);

  if ($rot !== false) {
    @unlink($source_file_path);
    write_image($rot, $source_file_path, $image_type);

    // A racer's record may refer to a cropped image which will have just been deleted; replace with
    // a path to the corresponding original.
    $cropped_file_path = $photo_repository->lookup(RENDER_CROPPED)->file_path($image_name);
    take_action('UPDATE RegistrationInfo'
                .' SET '.$photo_repository->column_name().' = :original'
                .' WHERE '.$photo_repository->column_name().' = :cropped',
                array(':original' => $source_file_path,
                      ':cropped' => $cropped_file_path));
    echo "<cache_breaker time='".time()."'/>\n";
  } else {
    echo "<failure code=\"unknown\">Rotation failed.</failure>\n";
  }

  imagedestroy($im);
  imagedestroy($rot);

  $original->delete_dependents($image_name);
}

end_response();

?>