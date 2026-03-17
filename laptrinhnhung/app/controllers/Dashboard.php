<?php
class Dashboard extends Controller {
    public function index() {
        // Gọi view dashboard
        $this->view('dashboard', ['title' => 'Smart Home Control']);
    }
}
?>