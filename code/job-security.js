// https://gist.githubusercontent.com/pedroreys/3455113/raw/b4dcdaffcc7bb3a1ee6bd0b16461783838c5d30c/job_security.js

функция f(x) {
  если (x % 2 == 0) {
    возврат истина;
  } иначе {
    возврат ложь;
  }
}

для (пер i = 0; i < 10; i++) {
  console.log(i, f(i));
  если (i > 5) прервать;
}

пер Terve = функция(nimi) {
  этот.nimi = nimi;
};
Terve.прототип = {};
Terve.прототип.moi = функция() {
  console.log( "Moi, " + этот.nimi + "!" );
};

пер tere = нов Terve("Veijo");

tere.moi();

для (пер i в tere) {
  console.log(i);
}
