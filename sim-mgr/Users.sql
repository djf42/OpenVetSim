-- phpMyAdmin SQL Dump
-- version 4.5.4.1deb2ubuntu2
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Aug 15, 2017 at 11:02 AM
-- Server version: 5.7.18-0ubuntu0.16.04.1
-- PHP Version: 7.0.18-0ubuntu0.16.04.1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `vet`
--

-- --------------------------------------------------------

--
-- Table structure for table `Users`
--

CREATE TABLE IF NOT EXISTS `Users` (
  `UserID` int(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `UserFirstName` varchar(50) NOT NULL,
  `UserLastName` varchar(50) NOT NULL,
  `UserEmail` varchar(50) NOT NULL,
  `UserPassWord` varchar(100) NOT NULL,
  `UserSalt` varchar(100) NOT NULL,
  `UserLastLogin` datetime DEFAULT NULL,
  PRIMARY KEY (`UserID`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=latin1;

--
-- Dumping data for table `Users`
--

INSERT INTO `Users` (`UserID`, `UserFirstName`, `UserLastName`, `UserEmail`, `UserPassWord`, `UserSalt`, `UserLastLogin`) VALUES
(5, 'Demo', 'User', 'no@email.com', 'nopass', 'nosalt', '2017-05-23 00:00:00'),
(6, 'admin', '', 'admin', 'ffbedb65195e4a72e7b43522eeecdbf616634cb4696e9f3027f87a8155cd1441', '8xb5z9f2zQnPBVAxUinRg3BeZDpgnrOlI7xXIkQPdLHjnM3SvoTvhHTZUeJx', '2017-08-15 10:57:11');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
